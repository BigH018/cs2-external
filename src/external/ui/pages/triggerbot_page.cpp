#include <imgui.h>

#include "config.h"
#include "features/triggerbot.h"
#include "settings/settings.h"
#include "ui/pages/controls.h"
#include "ui/pages/pages.h"
#include "ui/widgets.h"

namespace ui::pages
{
namespace
{
const char* block_text(features::TriggerBlock block)
{
    switch (block)
    {
    case features::TriggerBlock::none: return "on target";
    case features::TriggerBlock::not_in_match: return "not in a match (or dead)";
    case features::TriggerBlock::weapon: return "this weapon is filtered out";
    case features::TriggerBlock::not_scoped: return "sniper not scoped";
    case features::TriggerBlock::flashed: return "flashed";
    case features::TriggerBlock::in_air: return "in the air";
    case features::TriggerBlock::no_target: return "waiting for a target";
    }
    return "";
}

void draw_general(PageContext& ctx, settings::TriggerbotSettings& trigger)
{
    widgets::card_begin(ctx.fonts, "Triggerbot");
    check("Enabled", &trigger.enabled,
          "Fires for you while an enemy is under your crosshair (the game's own \"what am I aiming at\" value).");
    static constexpr const char* kActivations[] = {"Always on", "Hold key", "Toggle key"};
    ImGui::SetNextItemWidth(scaled(160.0f));
    combo("Activation", trigger.activation, kActivations, 3);
    if (trigger.activation != settings::TriggerActivation::always)
    {
        ImGui::SameLine(0.0f, scaled(24.0f));
        ImGui::SetNextItemWidth(scaled(140.0f));
        key_combo("Key", trigger.key);
    }
    ImGui::SameLine();
    widgets::help_marker("Always on: whenever it's enabled. Hold key: while the key is down. Toggle key: each press "
                         "switches it on or off. Every key arrives with the keybind engine (Phase 7).");
    if (trigger.enabled)
    {
        widgets::info_row("Status", !ctx.app.trigger_status.active ? "waiting for the key"
                                                                   : block_text(ctx.app.trigger_status.block));
    }
    widgets::card_end();
}

void draw_firing(PageContext& ctx, settings::TriggerbotSettings& trigger)
{
    widgets::card_begin(ctx.fonts, "Firing");
    ImGui::SetNextItemWidth(scaled(240.0f));
    ImGui::SliderInt("Reaction delay", &trigger.reaction_ms, config::kTriggerReaction.min,
                     config::kTriggerReaction.max, "%d ms", ImGuiSliderFlags_AlwaysClamp);
    ImGui::SameLine();
    widgets::help_marker("How long a target has to be under the crosshair before the first shot.");
    static constexpr const char* kModes[] = {"Single tap", "Burst", "Hold"};
    ImGui::SetNextItemWidth(scaled(160.0f));
    combo("Fire mode", trigger.fire_mode, kModes, 3);
    ImGui::SameLine();
    widgets::help_marker("Single tap: one shot, then the delay below. Burst: N shots the delay apart. Hold: attack "
                         "stays down while the target stays under the crosshair.");
    if (trigger.fire_mode == settings::FireMode::burst)
    {
        ImGui::SetNextItemWidth(scaled(240.0f));
        ImGui::SliderInt("Shots per burst", &trigger.burst_shots, config::kTriggerBurst.min,
                         config::kTriggerBurst.max, "%d", ImGuiSliderFlags_AlwaysClamp);
    }
    if (trigger.fire_mode != settings::FireMode::hold)
    {
        ImGui::SetNextItemWidth(scaled(240.0f));
        ImGui::SliderInt("Delay between shots", &trigger.shot_delay_ms, config::kTriggerShotDelay.min,
                         config::kTriggerShotDelay.max, "%d ms", ImGuiSliderFlags_AlwaysClamp);
        ImGui::SameLine();
        widgets::help_marker("Between the shots of a burst, and before the next tap or burst.");
    }
    widgets::card_end();
}

void draw_filters(PageContext& ctx, settings::TriggerbotSettings& trigger)
{
    widgets::card_begin(ctx.fonts, "Filters");
    team_mode_combo(ctx.app.settings.general.team_mode);
    check("Team check", &trigger.team_check, "Only fire at enemies (who is an enemy: the team mode above).");
    check("Visible only", &trigger.visible_only,
          "Only fire at bots the game marks as spotted by you (the radar's line-of-sight flag).");
    check("Head only", &trigger.head_only, "Only fire while the crosshair is on the head (from the bones).");
    max_distance_slider(trigger.max_distance, "Bots further away than this are ignored. All the way left = no limit.");
    check("Snipers: only when scoped", &trigger.snipers_scoped_only,
          "With an AWP, SSG 08, SCAR-20 or G3SG1, only fire while zoomed in.");
    check("Not while flashed", &trigger.not_flashed, "Hold fire while more than half of a flash is still on screen.");
    check("Not in the air", &trigger.not_in_air, "Hold fire while you're jumping or falling (shots go wide).");

    ImGui::TextUnformatted("Weapons");
    ImGui::SameLine();
    widgets::help_marker("Fire only with these weapon classes. Knives, grenades and the C4 never fire.");
    settings::WeaponFilter& weapons = trigger.weapons;
    const float column = scaled(130.0f);
    ImGui::Checkbox("Pistols", &weapons.pistol);
    ImGui::SameLine(column);
    ImGui::Checkbox("SMGs", &weapons.smg);
    ImGui::SameLine(column * 2.0f);
    ImGui::Checkbox("Rifles", &weapons.rifle);
    ImGui::Checkbox("Snipers", &weapons.sniper);
    ImGui::SameLine(column);
    ImGui::Checkbox("Shotguns", &weapons.shotgun);
    ImGui::SameLine(column * 2.0f);
    ImGui::Checkbox("Heavy", &weapons.heavy);
    widgets::card_end();
}
} // namespace

void draw_triggerbot(PageContext& ctx)
{
    widgets::page_header(ctx.fonts, "Triggerbot", "Fires when a living enemy bot is under the crosshair.");
    settings::TriggerbotSettings& trigger = ctx.app.settings.triggerbot;
    draw_general(ctx, trigger);
    draw_firing(ctx, trigger);
    draw_filters(ctx, trigger);
}
} // namespace ui::pages
