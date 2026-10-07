#include <imgui.h>

#include "config.h"
#include "settings/settings.h"
#include "ui/pages/controls.h"
#include "ui/pages/pages.h"
#include "ui/widgets.h"

namespace ui::pages
{
namespace
{
void draw_general(PageContext& ctx, settings::AimbotSettings& aim)
{
    widgets::card_begin(ctx.fonts, "Aimbot");
    check("Enabled", &aim.enabled,
          "Turns your view towards a bot while the aim key is held (or toggled on). It writes the game's view "
          "angles from outside, so the camera visibly moves: there is no silent aim externally.");
    ImGui::SetNextItemWidth(scaled(140.0f));
    key_combo("Aim key", aim.key);
    ImGui::SameLine(0.0f, scaled(24.0f));
    static constexpr const char* kModes[] = {"Hold", "Toggle"};
    ImGui::SetNextItemWidth(scaled(120.0f));
    combo("Mode", aim.mode, kModes, 2);
    ImGui::SameLine();
    widgets::help_marker("Hold: aims while the key is down. Toggle: each press switches aiming on or off. Mouse 1 "
                         "(the default) aims while you shoot. Every key arrives with the keybind engine (Phase 7).");
    const auto& status = ctx.app.aim_status;
    if (aim.enabled)
    {
        widgets::info_row("Status", !status.active ? "waiting for the aim key"
                                    : status.has_target ? "aiming"
                                                        : "active, nobody in the FOV");
    }
    widgets::card_end();
}

void draw_targeting(PageContext& ctx, settings::AimbotSettings& aim)
{
    widgets::card_begin(ctx.fonts, "Targeting");
    static constexpr const char* kTargets[] = {"Head", "Body", "Nearest"};
    ImGui::SetNextItemWidth(scaled(180.0f));
    combo("Aim at", aim.target, kTargets, 3);
    ImGui::SameLine();
    widgets::help_marker("Head: the middle of the head. Body: the chest. Nearest: whichever of head, neck, chest, "
                         "stomach and pelvis is closest to your crosshair.");
    static constexpr const char* kPriorities[] = {"Crosshair", "Distance", "Lowest health"};
    ImGui::SetNextItemWidth(scaled(180.0f));
    combo("Priority", aim.priority, kPriorities, 3);
    ImGui::SameLine();
    widgets::help_marker("Which bot to pick when several are inside the FOV: the one closest to your crosshair, the "
                         "closest to you, or the weakest.");
    team_mode_combo(ctx.app.settings.general.team_mode);
    check("Team check", &aim.team_check, "Only aim at enemies (who is an enemy: the team mode above).");
    check("Visible only", &aim.visible_only,
          "Only aim at bots the game marks as spotted by you (the radar's line-of-sight flag). It lags a little "
          "behind what you see.");
    max_distance_slider(aim.max_distance, "Bots further away than this are ignored. All the way left = no limit.");
    widgets::card_end();
}

void draw_feel(PageContext& ctx, settings::AimbotSettings& aim)
{
    widgets::card_begin(ctx.fonts, "Feel");
    ImGui::SetNextItemWidth(scaled(240.0f));
    ImGui::SliderFloat("FOV", &aim.fov, config::kAimFov.min, config::kAimFov.max, "%.1f deg",
                       ImGuiSliderFlags_AlwaysClamp);
    ImGui::SameLine();
    widgets::help_marker("How far from your crosshair a bot may be, in degrees, for the aimbot to take it.");
    check("Draw FOV circle", &aim.draw_fov, "The FOV as a circle around the crosshair.");
    if (aim.draw_fov)
    {
        ImGui::SameLine(0.0f, scaled(24.0f));
        colour("Circle colour", aim.fov_colour);
    }
    ImGui::SetNextItemWidth(scaled(240.0f));
    ImGui::SliderFloat("Smoothing", &aim.smoothing, config::kAimSmoothing.min, config::kAimSmoothing.max,
                       aim.smoothing <= 1.0f ? "snap" : "%.1f", ImGuiSliderFlags_AlwaysClamp);
    ImGui::SameLine();
    widgets::help_marker("1 = snap straight onto the target. Higher = a slower, smoother turn. It feels the same at "
                         "any frame rate.");
    widgets::card_end();
}
} // namespace

void draw_aimbot(PageContext& ctx)
{
    widgets::page_header(ctx.fonts, "Aimbot", "Turns your view towards a bot while the aim key is held.");
    settings::AimbotSettings& aim = ctx.app.settings.aimbot;
    draw_general(ctx, aim);
    draw_targeting(ctx, aim);
    draw_feel(ctx, aim);
}
} // namespace ui::pages
