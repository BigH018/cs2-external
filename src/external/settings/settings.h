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

// Who counts as an enemy, for every feature. Deathmatch is free-for-all in CS2 (everyone is an enemy whatever their
// team).
enum class TeamMode : std::uint8_t
{
    teams,        // enemies = the other team
    free_for_all, // enemies = everyone else
};

struct GeneralSettings
{
    TeamMode team_mode = TeamMode::teams;
};

// How a key switches a feature on. Phase 7's keybind engine adds PRESS and key capture; until then a key is chosen
// from input::kBindableKeys.
enum class BindMode : std::uint8_t
{
    hold,   // on while the key is held
    toggle, // each press switches it on or off
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
    bool show_teammates = false; // teams mode only: draw your own team too (in the team colours)
    float max_distance = 0.0f;   // metres; 0 = no limit (config::kMaxDistance)

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

// Which point of a target the aimbot aims at.
enum class AimTarget : std::uint8_t
{
    head,    // the head bone (the eyes without bones)
    body,    // the chest
    nearest, // whichever of head, neck, chest, stomach and pelvis is closest to the crosshair
};

// Which target the aimbot picks when several are inside the FOV.
enum class AimPriority : std::uint8_t
{
    crosshair,     // the smallest angle from where you're looking
    distance,      // the closest
    lowest_health, // the weakest
};

struct AimbotSettings
{
    bool enabled = false;
    std::uint32_t key = 0x01; // virtual-key code (input::kBindableKeys): Mouse 1, so it aims while you shoot
    BindMode mode = BindMode::hold;
    AimTarget target = AimTarget::head;
    AimPriority priority = AimPriority::crosshair;
    float fov = 5.0f;          // degrees from the crosshair (config::kAimFov)
    bool draw_fov = true;      // the FOV as a circle around the crosshair
    Color fov_colour = Color::rgb(0xEDEBF7, 0.35f);
    float smoothing = 5.0f;    // 1 = snap; higher = slower, framerate-independent (config::kAimSmoothing)
    bool team_check = true;    // only enemies (who is an enemy: GeneralSettings::team_mode)
    float max_distance = 0.0f; // metres; 0 = no limit (config::kMaxDistance)
    bool visible_only = false; // only targets the spotted-by heuristic says you can see
};

// When the triggerbot is allowed to fire.
enum class TriggerActivation : std::uint8_t
{
    always,
    hold,   // while the key is held
    toggle, // each press switches it on or off
};

enum class FireMode : std::uint8_t
{
    single, // one shot per volley
    burst,  // N shots per volley, "delay between shots" apart
    hold,   // attack held down while a target stays under the crosshair
};

// Which weapon classes the triggerbot fires with (game::WeaponClass).
struct WeaponFilter
{
    bool pistol = true;
    bool smg = true;
    bool rifle = true;
    bool sniper = true;
    bool shotgun = true;
    bool heavy = true;
};

struct TriggerbotSettings
{
    bool enabled = false;
    TriggerActivation activation = TriggerActivation::hold;
    std::uint32_t key = 0x05;  // virtual-key code (input::kBindableKeys): Mouse 4
    int reaction_ms = 40;      // from the target appearing to the first shot (config::kTriggerReaction)
    FireMode fire_mode = FireMode::single;
    int burst_shots = 3;       // burst mode (config::kTriggerBurst)
    int shot_delay_ms = 150;   // between shots, and after a volley (config::kTriggerShotDelay)
    bool team_check = true;    // only enemies (GeneralSettings::team_mode)
    bool visible_only = false; // only targets the spotted-by heuristic says you can see
    float max_distance = 0.0f; // metres; 0 = no limit (config::kMaxDistance)
    WeaponFilter weapons;
    bool snipers_scoped_only = true; // with a sniper rifle, only while zoomed in
    bool not_flashed = true;         // not while you're flashed
    bool not_in_air = true;          // not while you're jumping or falling
    bool head_only = false;          // only when the crosshair is on the head
};

// Which corner of the game window the radar sits in.
enum class RadarCorner : std::uint8_t
{
    top_left, // over the game's own radar (and the watermark: move one of them)
    top_right,
    bottom_left,
    bottom_right,
};

struct RadarColours
{
    Color enemy_visible = Color::rgb(0xF25C5C);
    Color enemy_hidden = Color::rgb(0xF2B35C);
    Color team = Color::rgb(0x5CA8F2);
    Color you = Color::rgb(0xEDEBF7);
    Color background = Color::rgb(0x0E1220, 0.72f);
};

// Our own radar, drawn by the overlay (not the game's): you in the middle, every player as a dot.
struct RadarSettings
{
    bool enabled = false;
    RadarCorner corner = RadarCorner::top_right;
    float size = 260.0f;         // pixels, the side of the square (config::kRadarSize)
    float range = 40.0f;         // metres from you to the edge (config::kRadarRange)
    bool rotate = true;          // where you look is up; off = the map's north (+y) is up, like the game's radar
    bool show_teammates = true;  // teams mode only
    bool facing = true;          // a short line from each dot the way that player looks
    bool names = false;
    bool clamp_to_edge = true;   // players out of range sit on the edge (faded) instead of disappearing
    float dot_size = 4.0f;       // pixels, a dot's radius (config::kRadarDotSize)
    bool visibility_colours = true; // enemy visible / hidden colours from the spotted-by heuristic
    RadarColours colours;
};

// A countdown panel at the top-centre of the game window while a bomb is planted.
struct BombTimerSettings
{
    bool enabled = false;
    float top = 120.0f;       // pixels from the top of the game window, under its round timer (config::kBombTimerTop)
    bool defuse_hint = true;  // whether a defuse started now would make it (no kit / kit / too late)
    bool distance = true;     // how far you are from the bomb
};

// Which side of the game window a panel sits on.
enum class PanelSide : std::uint8_t
{
    left,
    right,
};

// Who is watching you (or, while you're dead, the player you watch): dead players spectating in first or third person.
struct SpectatorSettings
{
    bool enabled = false;
    PanelSide side = PanelSide::right;
    float top = 290.0f;           // pixels from the top of the game window, under the radar (config::kSpectatorTop)
    bool show_mode = true;        // "1st person" / "3rd person" next to each name
    bool hide_when_empty = false; // no panel while nobody watches
};

struct Settings
{
    OverlaySettings overlay;
    GeneralSettings general;
    EspSettings esp;
    AimbotSettings aimbot;
    TriggerbotSettings triggerbot;
    RadarSettings radar;
    BombTimerSettings bomb_timer;
    SpectatorSettings spectators;
};
} // namespace settings
