#include <exception>
#include <iostream>
#include <string>
#include <string_view>

#include <Windows.h>

#include "app/diagnostics.h"
#include "app/frame.h"
#include "config.h"
#include "core/log.h"
#include "core/memory.h"
#include "core/process.h"
#include "core/process_memory.h"
#include "core/runtime.h"
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

// Runs on a thread Windows creates for the event. It only sets flags; the main thread does the shutdown.
BOOL WINAPI on_console_event(DWORD event) noexcept
{
    core::shutdown_requested = true;
    if (event == CTRL_C_EVENT || event == CTRL_BREAK_EVENT)
    {
        return TRUE; // handled: the main loop exits on its own
    }
    // Console closed, logoff, shutdown: Windows ends the process as soon as this returns, so give the main thread time
    // to remove the overlay and close the handle first.
    const ULONGLONG deadline = GetTickCount64() + config::kShutdownWaitMs;
    while (!core::shutdown_complete && GetTickCount64() < deadline)
    {
        Sleep(10);
    }
    return TRUE;
}

int run(bool diagnose_only)
{
    logger::info("CS2 External - Phase 2 (offline only: -insecure, bots, never a VAC server)");

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
        logger::info("local pawn   none (not in a match - join Practice with Bots)");
    }
    else if (!core::is_plausible_pointer(*pawn, alignof(std::uintptr_t)))
    {
        logger::warn("local pawn   0x{:X} is NOT a plausible pointer - dwLocalPlayerPawn may be stale", *pawn);
    }
    else
    {
        logger::info("local pawn   0x{:X}", *pawn);
    }

    const app::OffsetReport offsets = app::run_diagnostics(memory, *pid, *client, *engine);
    if (diagnose_only)
    {
        return offsets.ok() ? 0 : 2;
    }

    return app::run(app::Context{*pid, opened.handle.get(), memory, *client, *engine, offsets});
}
} // namespace

int main(int argc, char* argv[])
{
    // Before any window exists: overlay coordinates are then physical pixels, the same as the game's client area at
    // any Windows display scaling. Fails harmlessly if a manifest already set it.
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    SetConsoleCtrlHandler(&on_console_event, TRUE);

    int exit_code = 1;
    try
    {
        const bool diagnose_only = argc > 1 && std::string_view(argv[1]) == config::kDiagnoseFlag;
        exit_code = run(diagnose_only);
    }
    catch (const std::exception& e)
    {
        logger::error("Unhandled exception: {}", e.what());
    }
    core::shutdown_complete = true;
    if (!core::shutdown_requested)
    {
        wait_for_enter(); // keep the console readable when the tool was started by double-click
    }
    return exit_code;
}
