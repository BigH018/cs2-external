#include <imgui.h>

#include "color.h"
#include "config.h"
#include "settings/settings.h"
#include "ui/pages/controls.h"
#include "ui/pages/pages.h"
#include "ui/widgets.h"

namespace ui::pages
{
namespace
{
void draw_general(PageContext& ctx, settings::EspSettings& esp)
{
    widgets::card_begin(ctx.fonts, "ESP");
    check("Enabled", &esp.enabled,
          "Draws boxes and labels over the bots, through walls. Everything below applies live.");
    settings::TeamMode& team_mode = ctx.app.settings.general.team_mode;
    team_mode_combo(team_mode);
    if (team_mode == settings::TeamMode::teams)
    {
        check("Show teammates", &esp.show_teammates, "Draw your own team too, in the team colours.");
    }
    max_distance_slider(esp.max_distance, "Players further away than this aren't drawn. All the way left = no limit.");
    widgets::card_end();
}

void draw_shapes(PageContext& ctx, settings::EspSettings& esp)
{
    widgets::card_begin(ctx.fonts, "Shapes");
    check("Box", &esp.box, "A box from the feet to just above the head.");
    if (esp.box)
    {
        static constexpr const char* kStyles[] = {"Full", "Corners"};
        ImGui::SameLine(0.0f, scaled(24.0f));
        ImGui::SetNextItemWidth(scaled(140.0f));
        combo("Style", esp.box_style, kStyles, 2);
    }
    check("Outline", &esp.outline, "A dark edge around boxes, so they stand out on bright walls.");
    check("Head circle", &esp.head_circle,
          "A circle around the head (the middle of the head from the bones, or the eyes if bones can't be read).");
    check("Skeleton", &esp.skeleton, "Lines along the spine, arms and legs, from the bone positions.");
    check("Snaplines", &esp.snaplines, "A line from the edge or centre of the screen to each player.");
    if (esp.snaplines)
    {
        static constexpr const char* kOrigins[] = {"Bottom", "Centre", "Top"};
        ImGui::SameLine(0.0f, scaled(24.0f));
        ImGui::SetNextItemWidth(scaled(140.0f));
        combo("From", esp.snapline_origin, kOrigins, 3);
    }
    ImGui::SetNextItemWidth(scaled(240.0f));
    ImGui::SliderFloat("Thickness", &esp.thickness, config::kEspThickness.min, config::kEspThickness.max, "%.1f px",
                       ImGuiSliderFlags_AlwaysClamp);
    widgets::card_end();
}

void draw_labels(PageContext& ctx, settings::EspSettings& esp)
{
    widgets::card_begin(ctx.fonts, "Labels");
    check("Name", &esp.name, "The player's name above the box.");
    check("Health bar", &esp.health_bar, "A bar left of the box: green when healthy, red when nearly dead.");
    check("Health number", &esp.health_number, "The health as a number beside the bar.");
    check("Weapon", &esp.weapon, "The weapon in the player's hands, under the box.");
    check("Distance", &esp.distance, "How far away the player is, in metres.");
    check("Scoped indicator", &esp.scoped_indicator,
          "\"SCOPED\" above the name while the player is zoomed in (AWP, SSG 08, SCAR-20, G3SG1, AUG, SG 553).");
    widgets::card_end();
}

void draw_colours(PageContext& ctx, settings::EspSettings& esp)
{
    settings::EspColours& colours = esp.colours;
    widgets::card_begin(ctx.fonts, "Colours");
    check("Visible / hidden colours", &esp.visibility_colours,
          "Uses the game's own \"spotted\" flag (the one that puts players on the radar): visible while you have line "
          "of sight, hidden otherwise. It lags a little behind what you see. Off: always the visible colour.");
    colour("Enemy, visible", colours.enemy_visible);
    ImGui::SameLine(scaled(220.0f));
    colour("Enemy, hidden", colours.enemy_hidden);
    colour("Team, visible", colours.team_visible);
    ImGui::SameLine(scaled(220.0f));
    colour("Team, hidden", colours.team_hidden);
    colour("Text", colours.text);
    ImGui::SameLine(scaled(220.0f));
    colour("Skeleton", colours.skeleton);
    colour("Scoped tag", colours.scoped);
    if (ImGui::Button("Reset colours"))
    {
        colours = settings::EspColours{};
    }
    widgets::card_end();
}
} // namespace

void draw_esp(PageContext& ctx)
{
    widgets::page_header(ctx.fonts, "ESP", "Draws boxes and labels over the bots, through walls.");
    settings::EspSettings& esp = ctx.app.settings.esp;
    draw_general(ctx, esp);
    draw_shapes(ctx, esp);
    draw_labels(ctx, esp);
    draw_colours(ctx, esp);
}
} // namespace ui::pages
