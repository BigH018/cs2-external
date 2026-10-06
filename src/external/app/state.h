#pragma once

// Everything the app keeps between frames that the menu also shows or edits. Main thread only, so no locks.
// Plain data: the menu edits `overlay`; app/frame fills in the rest.

#include <cstdint>

#include "core/process.h"
#include "features/feature_summary.h"

namespace app
{
// What the bootstrap found (main.cpp), for the Home page.
struct GameInfo
{
    DWORD pid = 0;
    core::ModuleInfo client;
    core::ModuleInfo engine;
};

// Options for the overlay itself. Applied live. Phase 6 moves them into settings::Settings, Phase 8 saves them.
struct OverlayOptions
{
    bool watermark = true;      // logo, "External Cheat by BigH" and the active features, top-left
    bool frame_outline = false; // a thin outline along the overlay's edges, to check that it covers the game exactly
};

struct AppState
{
    GameInfo game;
    OverlayOptions overlay;

    // Which features are on, for the watermark. Filled from the settings by app/frame once features exist
    // (Phase 4+); all off until then.
    features::ActiveFeatures active;

    bool menu_open = false;
    bool focus_warning = false; // the menu opened but couldn't take focus from the game

    // Match status, refreshed every config::kStatusIntervalMs while the overlay is visible.
    bool pawn_read_ok = false;
    std::uintptr_t local_pawn = 0; // 0 = no local pawn (main menu, loading screen)

    int overlay_width = 0; // the game's client area, which the overlay covers
    int overlay_height = 0;
    float fps = 0.0f;      // overlay frames per second (ImGui's running average)

    [[nodiscard]] bool in_match() const noexcept { return pawn_read_ok && local_pawn != 0; }
};
} // namespace app
