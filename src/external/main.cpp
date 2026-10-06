#include <exception>
#include <iostream>
#include <string>
#include <string_view>

#include <Windows.h>

#include "config.h"
#include "core/log.h"
#include "core/memory.h"
#include "core/process.h"
#include "core/process_memory.h"
#include "game/player.h"

namespace
{
void wait_for_enter()
{
    std::cout << "\nPress Enter to exit..." << std::flush;
    std::string line;
    std::getline(std::cin, line);
}

void log_module(std::string_view name, const core::ModuleInfo& module)
{
    logger::info("{:<12} base 0x{:X}  size 0x{:X}", name, module.base, module.size);
}

int run()
{
    logger::info("CS2 External - Phase 0 (offline only: -insecure, bots, never a VAC server)");

    const auto pid = core::find_process(config::kGameExe);
    if (!pid)
    {
        logger::error("cs2.exe not found. Start CS2 with -insecure first.");
        return 1;
    }
    logger::info("cs2.exe      PID {}", *pid);

    const core::OpenResult opened = core::open_handle(*pid, core::kReadOnlyAccess);
    if (!opened.handle)
    {
        if (opened.error == ERROR_ACCESS_DENIED)
        {
            logger::error("OpenProcess failed (error 5, access denied). Run cs2_external.exe as administrator.");
        }
        else
        {
            logger::error("OpenProcess failed (error {}).", opened.error);
        }
        return 1;
    }
    const core::ProcessMemory memory(opened.handle.get());

    const auto client = core::module_base(*pid, config::kClientModule);
    const auto engine = core::module_base(*pid, config::kEngineModule);
    if (!client || !engine)
    {
        logger::error("client.dll / engine2.dll not found in cs2.exe. Wait for the main menu, then run again.");
        return 1;
    }
    log_module("client.dll", *client);
    log_module("engine2.dll", *engine);

    const auto pawn = game::read_local_pawn(memory, client->base);
    if (!pawn)
    {
        logger::error("Reading the local pawn pointer failed.");
        return 1;
    }
    if (*pawn == 0)
    {
        logger::info("local pawn   none (not in a match - join Practice with Bots and run again)");
    }
    else if (!core::is_plausible_pointer(*pawn, alignof(std::uintptr_t)))
    {
        logger::warn("local pawn   0x{:X} is NOT a plausible pointer - dwLocalPlayerPawn may be stale", *pawn);
    }
    else
    {
        logger::info("local pawn   0x{:X}", *pawn);
    }
    return 0;
}
} // namespace

int main()
{
    int exit_code = 1;
    try
    {
        exit_code = run();
    }
    catch (const std::exception& e)
    {
        logger::error("Unhandled exception: {}", e.what());
    }
    wait_for_enter();
    return exit_code;
}
