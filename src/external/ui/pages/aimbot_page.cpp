#include "ui/pages/pages.h"
#include "ui/widgets.h"

namespace ui::pages
{
void draw_aimbot(PageContext& ctx)
{
    widgets::page_header(ctx.fonts, "Aimbot", "Turns your view towards a bot while the aim key is held.");
    widgets::planned_card(ctx.fonts, "Phase 5",
                          {"Hold / toggle aim key", "Head or body target", "Priority: crosshair, distance, lowest health",
                           "FOV radius and circle", "Smoothing", "Team check, max distance",
                           "Visible only (spotted-by heuristic)"});
}
} // namespace ui::pages
