#pragma once

// Every user setting, as plain data. The menu edits it; app/frame reads it every frame, so changes apply live.
// Numeric ranges live in config.h (config::Range); Phase 8 saves and loads this as JSON profiles.
//
// PURE: no <Windows.h>, no ImGui.

#include <cstdint>

#include "color.h"

namespace settings
{
// The overlay itself (moved here from app::OverlayOptions in Phase 4).
struct OverlaySettings
{
    bool watermark = true;      // logo, "External Cheat by BigH" and the active features, top-left
    bool frame_outline = false; // a thin outline along the overlay's edges, to check that it covers the game exactly
};

// Who counts as an enemy. Deathmatch is free-for-all in CS2 (everyone is an enemy whatever their team).
enum class TeamMode : std::uint8_t
{
    teams,        // enemies = the other team
    free_for_all, // enemies = everyone else
};

enum class BoxStyle : std::uint8_t
{
    full,    // a whole rectangle
    corners, // only the four corners
};

// Where snaplines start.
enum class SnaplineOrigin : std::uint8_t
{
    bottom, // the middle of the bottom edge of the screen
    centre, // the crosshair
    top,    // the middle of the top edge
};

struct EspColours
{
    Color enemy_visible = Color::rgb(0xF25C5C);
    Color enemy_hidden = Color::rgb(0xF2B35C);
    Color team_visible = Color::rgb(0x5CD6A0);
    Color team_hidden = Color::rgb(0x5CA8F2);
    Color skeleton = Color::rgb(0xEDEBF7, 0.9f);
    Color text = Color::rgb(0xEDEBF7);
    Color scoped = Color::rgb(0xC6BEEB);
};

struct EspSettings
{
    bool enabled = false;
    TeamMode team_mode = TeamMode::teams;
    bool show_teammates = false; // teams mode only: draw your own team too (in the team colours)
    float max_distance = 0.0f;   // metres; 0 = no limit (config::kEspMaxDistance)

    bool box = true;
    BoxStyle box_style = BoxStyle::full;
    float thickness = 1.5f; // boxes, lines and circles, in pixels (config::kEspThickness)
    bool outline = true;    // a dark edge around boxes, for contrast on bright maps
    bool head_circle = false;
    bool skeleton = false;

    bool name = true;
    bool health_bar = true;
    bool health_number = false;
    bool distance = true;
    bool weapon = true;
    bool scoped_indicator = true; // "SCOPED" over a bot that is zoomed in

    bool snaplines = false;
    SnaplineOrigin snapline_origin = SnaplineOrigin::bottom;

    bool visibility_colours = true; // visible / hidden colours from the spotted-by heuristic (game/visibility)
    EspColours colours;
};

struct Settings
{
    OverlaySettings overlay;
    EspSettings esp;
};
} // namespace settings
