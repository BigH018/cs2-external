#pragma once

// The orchestrator (same role as in the AC project): the only place that wires the overlay window, ImGui, game reads
// and the menu together, one frame at a time. Main thread only.
//
// One frame: pump messages -> follow the game window and focus -> menu key -> game snapshot (every frame while the
// ESP, aimbot or triggerbot is on, ~4 Hz otherwise) -> aimbot (view angle write) -> triggerbot (attack button write)
// -> ESP + FOV circle -> HUD + menu -> present. Phase 7 replaces the key polling with the keybind engine.

#include <Windows.h>

#include "app/state.h"
#include "core/memory.h"
#include "core/process.h"

namespace app
{
// What the bootstrap hands over. `process` and `memory` stay valid until run() returns.
struct Context
{
    DWORD pid = 0;
    HANDLE process = nullptr; // for is_running (PROCESS_QUERY_LIMITED_INFORMATION)
    core::Memory& memory; // read-write: the aimbot and triggerbot write single fields (game/writes)
    core::ModuleInfo client;
    core::ModuleInfo engine;
    OffsetReport offsets; // the startup diagnostic's result, for the Home page
};

// Runs the overlay until the user exits (core::shutdown_requested or Alt+F4 on the menu), the game closes, or the
// overlay fails. Tears the overlay down before returning. Returns the process exit code.
int run(const Context& ctx);
} // namespace app
