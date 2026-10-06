#include "ui/pages/pages.h"
#include "ui/widgets.h"

namespace ui::pages
{
void draw_esp(PageContext& ctx)
{
    widgets::page_header(ctx.fonts, "ESP", "Draws boxes and labels over the bots, through walls.");
    widgets::planned_card(ctx.fonts, "Phase 4",
                          {"2D box, corner box, head circle", "Skeleton (once the bone array is found)",
                           "Name, health bar / number, distance, weapon", "Scoped indicator (AWP, SSG 08, autos...)",
                           "Visible / hidden colours (spotted-by heuristic)", "Snaplines (choice of origin)",
                           "Team mode, enemies only", "Colours with opacity, thickness"});
}
} // namespace ui::pages
