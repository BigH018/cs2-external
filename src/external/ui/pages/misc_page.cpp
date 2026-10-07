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
void draw_radar(PageContext& ctx, settings::RadarSettings& radar)
{
    widgets::card_begin(ctx.fonts, "Radar");
    check("Enabled", &radar.enabled,
          "Our own radar, drawn by the overlay: you in the middle, every player as a dot, through walls. The game's "
          "radar isn't touched.");
    static constexpr const char* kCorners[] = {"Top left", "Top right", "Bottom left", "Bottom right"};
    ImGui::SetNextItemWidth(scaled(180.0f));
    combo("Corner", radar.corner, kCorners, 4);
    ImGui::SameLine();
    widgets::help_marker("Where the radar sits. Top left covers the game's own radar and the watermark.");
    ImGui::SetNextItemWidth(scaled(240.0f));
    ImGui::SliderFloat("Size", &radar.size, config::kRadarSize.min, config::kRadarSize.max, "%.0f px",
                       ImGuiSliderFlags_AlwaysClamp);
    ImGui::SetNextItemWidth(scaled(240.0f));
    ImGui::SliderFloat("Range", &radar.range, config::kRadarRange.min, config::kRadarRange.max, "%.0f m",
                       ImGuiSliderFlags_AlwaysClamp);
    ImGui::SameLine();
    widgets::help_marker("How far the radar reaches, from you to its edge. Smaller = zoomed in.");
    ImGui::SetNextItemWidth(scaled(240.0f));
    ImGui::SliderFloat("Dot size", &radar.dot_size, config::kRadarDotSize.min, config::kRadarDotSize.max, "%.1f px",
                       ImGuiSliderFlags_AlwaysClamp);
    check("Rotate with view", &radar.rotate,
          "On: where you look is always up. Off: the map stays still (north up, like the game's radar) and your "
          "arrow turns.");
    settings::TeamMode& team_mode = ctx.app.settings.general.team_mode;
    team_mode_combo(team_mode);
    if (team_mode == settings::TeamMode::teams)
    {
        check("Show teammates", &radar.show_teammates, "Draw your own team too, in the team colour.");
    }
    check("Facing", &radar.facing, "A short line from each dot, the way that player is looking.");
    check("Names", &radar.names, "Each player's name under their dot.");
    check("Keep out-of-range players on the edge", &radar.clamp_to_edge,
          "Players further away than the range sit on the edge in their direction, faded. Off: they're hidden.");
    widgets::card_end();
}

void draw_radar_colours(PageContext& ctx, settings::RadarSettings& radar)
{
    settings::RadarColours& colours = radar.colours;
    widgets::card_begin(ctx.fonts, "Radar colours");
    check("Visible / hidden colours", &radar.visibility_colours,
          "Enemies you can see (the game's \"spotted\" flag, which lags a little) in the visible colour, the rest in "
          "the hidden one. Off: every enemy in the visible colour.");
    colour("Enemy, visible", colours.enemy_visible);
    ImGui::SameLine(scaled(220.0f));
    colour("Enemy, hidden", colours.enemy_hidden);
    colour("Team", colours.team);
    ImGui::SameLine(scaled(220.0f));
    colour("You", colours.you);
    colour("Background", colours.background);
    if (ImGui::Button("Reset colours"))
    {
        colours = settings::RadarColours{};
    }
    widgets::card_end();
}

void draw_bomb_timer(PageContext& ctx, settings::BombTimerSettings& timer)
{
    widgets::card_begin(ctx.fonts, "Bomb timer");
    check("Enabled", &timer.enabled,
          "While a bomb is planted: a countdown at the top of the screen with the site, a bar, and who is defusing. "
          "Only in modes with a bomb (Casual, Competitive), not Deathmatch.");
    ImGui::SetNextItemWidth(scaled(240.0f));
    ImGui::SliderFloat("Height", &timer.top, config::kBombTimerTop.min, config::kBombTimerTop.max,
                       "%.0f px from the top", ImGuiSliderFlags_AlwaysClamp);
    check("Defuse hint", &timer.defuse_hint,
          "Whether a defuse started now would make it: green = even without a kit (over 10 s left), yellow = only "
          "with a kit (over 5 s), red = too late. While someone defuses: whether their defuse finishes in time.");
    check("Distance", &timer.distance, "How far you are from the bomb.");
    widgets::card_end();
}
} // namespace

void draw_misc(PageContext& ctx)
{
    widgets::page_header(ctx.fonts, "Misc", "Radar and match helpers.");
    settings::RadarSettings& radar = ctx.app.settings.radar;
    draw_radar(ctx, radar);
    draw_radar_colours(ctx, radar);
    draw_bomb_timer(ctx, ctx.app.settings.bomb_timer);
    widgets::planned_card(ctx.fonts, "Coming next (Phase 6)",
                          {"Spectator list", "Hitsound (played by the overlay, not the game)"});
}
} // namespace ui::pages
