#include "ui/pages/pages.h"
#include "ui/widgets.h"

namespace ui::pages
{
void draw_triggerbot(PageContext& ctx)
{
    widgets::page_header(ctx.fonts, "Triggerbot", "Fires when a living enemy bot is under the crosshair.");
    widgets::planned_card(ctx.fonts, "Phase 5",
                          {"Activation: always on, hold key or toggle key", "Reaction delay before the first shot",
                           "Fire mode: single tap, burst (N shots) or hold while on target",
                           "Delay between shots", "Team check, visible only, max distance",
                           "Weapon filter by class (pistol, SMG, rifle, sniper, shotgun, heavy)",
                           "Snipers: only fire when scoped", "Don't fire while flashed or in the air",
                           "Head only (once the bone array is found)"});
}
} // namespace ui::pages
