#pragma once

// The orchestrator (same role as in the AC project): the only place that wires the overlay window, ImGui, game reads
// and the menu together, one frame at a time. Main thread only.
//
// One frame (Phase 1): pump messages -> follow the game window and focus -> menu key -> status read (~4 Hz) -> HUD +
// menu -> present. Later phases slot keybinds, game snapshots, features and writes in between (CLAUDE.md §6.3).

#include <Windows.h>

#include "core/memory.h"
#include "core/process.h"

namespace app
{
// What the bootstrap hands over. `process` and `memory` stay valid until run() returns.
struct Context
{
    DWORD pid = 0;
    HANDLE process = nullptr; // for is_running (PROCESS_QUERY_LIMITED_INFORMATION)
    const core::Memory& memory;
    core::ModuleInfo client;
    core::ModuleInfo engine;
};

// Runs the overlay until the user exits (core::shutdown_requested or Alt+F4 on the menu), the game closes, or the
// overlay fails. Tears the overlay down before returning. Returns the process exit code.
int run(const Context& ctx);
} // namespace app
