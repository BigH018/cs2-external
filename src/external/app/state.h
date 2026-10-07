#pragma once

// Everything the app keeps between frames that the menu also shows or edits. Main thread only, so no locks.
// Plain data: the menu edits `settings`; app/frame fills in the rest.

#include <cstdint>

#include "core/process.h"
#include "features/feature_summary.h"
#include "features/triggerbot.h"
#include "game/snapshot.h"
#include "settings/settings.h"

namespace app
{
// What the bootstrap found (main.cpp), for the Home page.
struct GameInfo
{
    DWORD pid = 0;
    core::ModuleInfo client;
    core::ModuleInfo engine;
};

// The startup offset diagnostic's result (app/diagnostics), for the Home page. The details are in the console.
struct OffsetReport
{
    bool ran = false;
    std::uint32_t game_build = 0; // engine2.dll + dwBuildNumber; 0 = couldn't be read
    int checks = 0;
    int failures = 0;

    [[nodiscard]] bool ok() const noexcept { return ran && failures == 0; }
};

struct AppState
{
    GameInfo game;
    OffsetReport offsets;
    settings::Settings settings; // edited by the menu, applied live by app/frame

    // Which features are on, for the watermark. Filled from the settings by app/frame every frame.
    features::ActiveFeatures active;

    // What the aimbot and triggerbot are doing this frame, for their pages.
    struct AimStatus
    {
        bool active = false;     // enabled, key held / toggled on, menu closed, game focused
        bool has_target = false; // a target inside the FOV this frame
    } aim_status;
    struct TriggerStatus
    {
        bool active = false; // enabled, activation allows it, menu closed, game focused
        features::TriggerBlock block = features::TriggerBlock::not_in_match;
    } trigger_status;

    bool menu_open = false;
    bool focus_warning = false; // the menu opened but couldn't take focus from the game

    // Match status, refreshed every config::kStatusIntervalMs while the overlay is visible.
    bool pawn_read_ok = false;
    std::uintptr_t local_pawn = 0; // 0 = no local pawn (main menu, loading screen)
    game::GameSnapshot snapshot;   // every player, globals, view matrix: every frame while a feature needs it

    int overlay_width = 0; // the game's client area, which the overlay covers
    int overlay_height = 0;
    float fps = 0.0f;      // overlay frames per second (ImGui's running average)

    [[nodiscard]] bool in_match() const noexcept { return pawn_read_ok && local_pawn != 0; }
};
} // namespace app
