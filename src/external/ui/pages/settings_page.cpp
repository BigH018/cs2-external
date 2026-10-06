#include <imgui.h>

#include "ui/pages/pages.h"
#include "ui/widgets.h"

namespace ui::pages
{
void draw_settings(PageContext& ctx)
{
    widgets::page_header(ctx.fonts, "Settings", "Applied immediately. Saving them in profiles comes in Phase 8.");

    widgets::card_begin(ctx.fonts, "Overlay");
    ImGui::Checkbox("Watermark", &ctx.app.overlay.watermark);
    ImGui::SameLine();
    widgets::help_marker("The logo and \"External Cheat by BigH\" in the top-left corner of the game, with the "
                         "features that are on listed under it.");
    ImGui::Checkbox("Frame outline", &ctx.app.overlay.frame_outline);
    ImGui::SameLine();
    widgets::help_marker("A thin line along the overlay's edges. If the overlay covers the game exactly, the line sits "
                         "right on the edges of the game's picture, in windowed mode too.");
    widgets::card_end();

    widgets::planned_card(ctx.fonts, "Phases 7-10",
                          {"Keybinds for every action (Phase 7)", "Profiles and presets (Phase 8)",
                           "Panic and exit keys (Phase 9)", "Menu size (Phase 10)"});
}
} // namespace ui::pages
