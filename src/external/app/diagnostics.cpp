#include "app/diagnostics.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <format>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "config.h"
#include "core/log.h"
#include "core/pattern.h"
#include "core/pe.h"
#include "game/interfaces.h"
#include "game/offsets.h"
#include "game/schema.h"
#include "game/schema_system.h"
#include "game/signatures.h"

namespace app
{
namespace
{
namespace offsets = game::offsets;

class Tally
{
public:
    // One proven-or-not check: printed as OK or FAIL and counted.
    void check(bool ok, const std::string& line)
    {
        ++checks_;
        if (ok)
        {
            logger::info("  OK   {}", line);
        }
        else
        {
            ++failures_;
            logger::warn("  FAIL {}", line);
        }
    }

    [[nodiscard]] int checks() const noexcept { return checks_; }
    [[nodiscard]] int failures() const noexcept { return failures_; }

private:
    int checks_ = 0;
    int failures_ = 0;
};

std::string hex_or(const std::optional<std::uintptr_t>& value, std::string_view otherwise)
{
    return value ? std::format("0x{:X}", *value) : std::string(otherwise);
}

// --- 1. Build ------------------------------------------------------------------------------------------------------
std::uint32_t check_build(Tally& tally, const core::Memory& memory, const core::ModuleInfo& engine)
{
    logger::info("Build");
    const auto build = memory.read<std::uint32_t>(engine.base + offsets::engine2::dwBuildNumber);
    if (!build)
    {
        tally.check(false, "engine2.dll + dwBuildNumber can't be read");
        return 0;
    }
    tally.check(*build == offsets::kDumpBuildNumber,
                std::format("game build {} (engine2.dll + dwBuildNumber), dumps from build {}", *build,
                            offsets::kDumpBuildNumber));
    if (*build != offsets::kDumpBuildNumber)
    {
        logger::warn("       CS2 was updated after the dump: re-run a2x/cs2-dumper (CLAUDE.md §13, after every update)");
    }
    return *build;
}

// --- 2. Interfaces -------------------------------------------------------------------------------------------------
struct WantedInterface
{
    std::string_view module_label;
    std::uintptr_t module_base;
    std::string_view name;
    std::uintptr_t dumped_rva;
};

// Returns SchemaSystem_001's instance (the schema check needs it).
std::optional<std::uintptr_t> check_interfaces(Tally& tally, const core::Memory& memory, DWORD pid,
                                               const core::ModuleInfo& client, const core::ModuleInfo& engine)
{
    logger::info("Interfaces (walked from each module's CreateInterface -> InterfaceReg list)");
    const auto schema_module = core::module_base(pid, config::kSchemaSystemModule);
    const auto input_module = core::module_base(pid, config::kInputSystemModule);

    const std::array wanted{
        WantedInterface{"client.dll", client.base, "Source2Client002", offsets::interfaces::Source2Client002},
        WantedInterface{"engine2.dll", engine.base, "Source2EngineToClient001",
                        offsets::interfaces::Source2EngineToClient001},
        WantedInterface{"schemasystem.dll", schema_module ? schema_module->base : 0, "SchemaSystem_001",
                        offsets::interfaces::SchemaSystem_001},
        WantedInterface{"inputsystem.dll", input_module ? input_module->base : 0, "InputSystemVersion001",
                        offsets::interfaces::InputSystemVersion001},
    };

    std::optional<std::uintptr_t> schema_system;
    std::optional<std::uintptr_t> input_system;
    for (const WantedInterface& want : wanted)
    {
        if (want.module_base == 0)
        {
            tally.check(false, std::format("{:<26} {} not loaded", want.name, want.module_label));
            continue;
        }
        const auto list = game::list_interfaces(memory, want.module_base);
        if (!list)
        {
            tally.check(false, std::format("{:<26} {}: CreateInterface not found or not as expected", want.name,
                                           want.module_label));
            continue;
        }
        const auto instance = game::find_interface(*list, want.name);
        const std::optional<std::uintptr_t> rva =
            instance ? std::optional<std::uintptr_t>(*instance - want.module_base) : std::nullopt;
        tally.check(rva == want.dumped_rva,
                    std::format("{:<26} {} -> {}  ({}+{}, dump +0x{:X}; the module registers {})", want.name,
                                want.module_label, hex_or(instance, "not registered"), want.module_label,
                                hex_or(rva, "?"), want.dumped_rva, list->size()));
        if (want.name == "SchemaSystem_001")
        {
            schema_system = instance;
        }
        if (want.name == "InputSystemVersion001")
        {
            input_system = instance;
        }
    }

    if (input_module)
    {
        const std::uintptr_t global = input_module->base + offsets::inputsystem::dwInputSystem;
        tally.check(input_system == global,
                    std::format("{:<26} inputsystem.dll + dwInputSystem = 0x{:X} is the InputSystemVersion001 instance",
                                "dwInputSystem", global));
    }

    // Not an interface in CS2: a global pointer in client.dll.
    const auto entity_system = memory.read<std::uintptr_t>(client.base + offsets::client::dwGameEntitySystem);
    const auto highest = entity_system ? memory.read<std::int32_t>(*entity_system +
                                                                   offsets::client::dwGameEntitySystem_highestEntityIndex)
                                       : std::nullopt;
    const bool entity_system_ok = entity_system && core::is_plausible_pointer(*entity_system, 8) && highest &&
                                  *highest >= 0 && *highest < 0x8000;
    tally.check(entity_system_ok,
                std::format("{:<26} client.dll + dwGameEntitySystem -> {}, highest entity index {}",
                            "GameEntitySystem", hex_or(entity_system, "unreadable"),
                            highest ? std::to_string(*highest) : std::string("unreadable")));
    return schema_system;
}

// --- 3. Signatures -------------------------------------------------------------------------------------------------
void check_signatures(Tally& tally, const core::RemoteCopy& client_copy, const std::optional<core::pe::Section>& text)
{
    logger::info("Signatures (client.dll .text)");
    for (const offsets::Signature& signature : offsets::signatures::kClient)
    {
        if (!text)
        {
            tally.check(false, std::format("{:<26} no .text section in client.dll", signature.name));
            continue;
        }
        const game::SignatureResult result = game::resolve_signature(client_copy, *text, signature);
        const bool found = result.status == game::SignatureStatus::found;
        tally.check(found && result.rva == signature.dumped,
                    std::format("{:<26} {:<12} {} hit{}{}  (dump 0x{:X})", signature.name,
                                found ? std::format("0x{:X}", result.rva) : std::string(game::to_string(result.status)),
                                result.hits, result.hits == 1 ? "" : "s",
                                signature.add != 0 ? std::format(", +0x{:X}", signature.add) : std::string(),
                                signature.dumped));
    }
}

// --- 4. Schema -----------------------------------------------------------------------------------------------------
void check_schema(Tally& tally, const core::Memory& memory, std::optional<std::uintptr_t> schema_system,
                  const core::RemoteCopy& client_copy)
{
    logger::info("Schema (the game's live schema system vs game/schema.h)");
    const auto scope = schema_system ? game::find_type_scope(memory, *schema_system, config::kClientTypeScope)
                                     : std::nullopt;
    const auto classes = game::index_classes(client_copy, config::kClientSchemaModule);
    tally.check(scope.has_value() && !classes.empty(),
                std::format("{:<26} type scope \"{}\" at {}, {} client classes found in client.dll", "SchemaSystem_001",
                            config::kClientTypeScope, hex_or(scope, "not found"), classes.size()));

    std::map<std::string_view, std::optional<game::SchemaClass>> read_classes;
    for (const game::schema::Field& field : game::schema::kFields)
    {
        auto [entry, inserted] = read_classes.try_emplace(field.class_name);
        if (inserted)
        {
            const auto info = classes.find(std::string(field.class_name));
            if (info != classes.end())
            {
                entry->second = game::read_class(memory, info->second);
            }
        }
        const std::string name = std::format("{}::{}", field.class_name, field.field_name);
        const std::optional<game::SchemaClass>& live_class = entry->second;
        if (!live_class)
        {
            tally.check(false, std::format("{:<44} class not found in the live schema", name));
            continue;
        }
        const auto live = live_class->offset_of(field.field_name);
        const bool in_scope = !scope || live_class->type_scope == *scope;
        tally.check(live == field.offset && in_scope,
                    std::format("{:<44} 0x{:<6X} live {}{}", name, field.offset,
                                live ? std::format("0x{:X}", *live) : std::string("missing"),
                                in_scope ? "" : "  (class is in another type scope)"));
    }
}

// --- 5. Buttons and globals (informational) ------------------------------------------------------------------------
void show_globals(const core::Memory& memory, const core::ModuleInfo& client)
{
    logger::info("Buttons and globals (as they read right now; not checks)");
    struct Button
    {
        std::string_view name;
        std::uintptr_t rva;
    };
    for (const Button& button : {Button{"jump", offsets::buttons::jump}, Button{"attack", offsets::buttons::attack}})
    {
        const std::uintptr_t address = client.base + button.rva;
        const auto state = memory.read<std::uint32_t>(address);
        logger::info("       {:<26} client.dll+0x{:X} = 0x{:X}  state 0x{:08X}", button.name, button.rva, address,
                     state.value_or(0));
    }

    struct Global
    {
        std::string_view name;
        std::uintptr_t rva;
    };
    const std::array pointers{
        Global{"dwLocalPlayerController", offsets::client::dwLocalPlayerController},
        Global{"dwLocalPlayerPawn", offsets::client::dwLocalPlayerPawn},
        Global{"dwGlobalVars", offsets::client::dwGlobalVars},
        Global{"dwGameRules", offsets::client::dwGameRules},
        Global{"dwPlantedC4", offsets::client::dwPlantedC4},
    };
    for (const Global& global : pointers)
    {
        const auto value = memory.read<std::uintptr_t>(client.base + global.rva);
        logger::info("       {:<26} client.dll+0x{:X} -> {}", global.name, global.rva,
                     value ? (*value == 0 ? std::string("0 (none right now)") : std::format("0x{:X}", *value))
                           : std::string("unreadable"));
    }

    std::array<float, 16> matrix{};
    const bool read = memory.read_bytes(client.base + offsets::client::dwViewMatrix, matrix.data(), sizeof(matrix));
    const bool all_zero = read && std::all_of(matrix.begin(), matrix.end(), [](float v) { return v == 0.0f; });
    logger::info("       {:<26} client.dll+0x{:X}: {}", "dwViewMatrix (4x4 floats)", offsets::client::dwViewMatrix,
                 !read ? std::string("unreadable")
                 : all_zero
                     ? std::string("all zero (no frame drawn yet)")
                     : std::format("row 0 = {:.3f} {:.3f} {:.3f} {:.3f}", matrix[0], matrix[1], matrix[2], matrix[3]));
}
} // namespace

OffsetReport run_diagnostics(const core::Memory& memory, DWORD pid, const core::ModuleInfo& client,
                             const core::ModuleInfo& engine)
{
    logger::info("--- Offset diagnostic (dumps: CS2 build {}, a2x/cs2-dumper) ---", offsets::kDumpBuildNumber);
    Tally tally;
    OffsetReport report;
    report.ran = true;
    report.game_build = check_build(tally, memory, engine);
    const auto schema_system = check_interfaces(tally, memory, pid, client, engine);

    // One copy of client.dll for both the signature scan and the schema class index (freed on return).
    const auto started = std::chrono::steady_clock::now();
    const core::RemoteCopy client_copy = core::copy_remote(memory, client.base, client.size);
    const auto elapsed =
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - started);
    logger::info("client.dll copied for scanning: {} MiB in {} ms ({} unreadable pages)",
                 client_copy.bytes.size() / (1024 * 1024), elapsed.count(), client_copy.unreadable_pages);
    const auto headers = core::pe::read_headers(memory, client.base);
    const auto text = headers ? core::pe::find_section(*headers, ".text") : std::nullopt;

    check_signatures(tally, client_copy, text);
    check_schema(tally, memory, schema_system, client_copy);
    show_globals(memory, client);

    report.checks = tally.checks();
    report.failures = tally.failures();
    if (report.failures == 0)
    {
        logger::info("--- Offset diagnostic: all {} checks OK ---", report.checks);
    }
    else
    {
        logger::warn("--- Offset diagnostic: {} of {} checks FAILED (details above) ---", report.failures,
                     report.checks);
    }
    return report;
}
} // namespace app
