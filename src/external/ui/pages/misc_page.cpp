#include "ui/pages/pages.h"
#include "ui/widgets.h"

namespace ui::pages
{
void draw_misc(PageContext& ctx)
{
    widgets::page_header(ctx.fonts, "Misc", "Movement, radar and match helpers.");
    widgets::planned_card(ctx.fonts, "Phase 6",
                          {"Bunny hop: hold space, it times each jump for you", "Enlarged radar with enemy dots",
                           "Bomb timer", "Spectator list", "Hitsound (played by the overlay, not the game)"});
}
} // namespace ui::pages
