#include <format>
#include <string>

#include <imgui.h>

#include "config.h"
#include "features/feature_summary.h"
#include "ui/pages/pages.h"
#include "ui/widgets.h"

namespace ui::pages
{
namespace
{
std::string module_text(const core::ModuleInfo& module)
{
    return std::format("0x{:X}  (size 0x{:X})", module.base, module.size);
}

void draw_status(PageContext& ctx)
{
    const app::AppState& app = ctx.app;
    widgets::card_begin(ctx.fonts, "Status");
    widgets::info_row("Game", std::format("cs2.exe  (PID {})", app.game.pid).c_str());
    widgets::info_row("client.dll", module_text(app.game.client).c_str());
    widgets::info_row("engine2.dll", module_text(app.game.engine).c_str());
    if (!app.pawn_read_ok)
    {
        widgets::info_row("Match", "local pawn read failed", theme::kWarn);
    }
    else if (app.local_pawn == 0)
    {
        widgets::info_row("Match", "not in a match", theme::kTextDim);
    }
    else
    {
        widgets::info_row("Match", std::format("in a match  (local pawn 0x{:X})", app.local_pawn).c_str(), theme::kOk);
    }
    const std::string active = features::feature_summary(app.active);
    widgets::info_row("Active", active.empty() ? "no features on" : active.c_str(),
                      active.empty() ? theme::kTextDim : theme::kAccent);
    widgets::info_row("Overlay",
                      std::format("{} x {}  at {:.0f} FPS", app.overlay_width, app.overlay_height, app.fps).c_str());
    widgets::card_end();
}

void draw_how_to(PageContext& ctx)
{
    widgets::card_begin(ctx.fonts, "Using the overlay");
    widgets::hint(std::format("{} opens and closes this menu. While it's open the game doesn't get your mouse or "
                              "keyboard, so clicks here never shoot.",
                              config::kMenuToggleKeyName)
                      .c_str());
    widgets::hint("The overlay is a separate, click-through window on top of the game. It hides when the game loses "
                  "focus (Alt+Tab) and follows the game window when it moves or changes resolution.");
    widgets::hint("To exit: Ctrl+C in the console window (or close it), or Alt+F4 while the menu is open. The game "
                  "keeps running.");
    widgets::card_end();
}
} // namespace

void draw_home(PageContext& ctx)
{
    // The logo with the name beside it, centred on the logo's height.
    const float logo = scaled(config::kHomeLogoSize);
    const float top = ImGui::GetCursorPosY();
    widgets::image_rounded(ctx.logo, logo, logo * 0.2f);
    const float below = ImGui::GetCursorPosY();
    ImGui::SameLine(0.0f, scaled(16.0f));
    ImGui::PushFont(ctx.fonts.bold, scaled(config::kPageTitleFontSize));
    ImGui::SetCursorPosY(top + (logo - ImGui::GetTextLineHeight()) * 0.5f);
    ImGui::TextUnformatted(config::kAppName);
    ImGui::PopFont();
    ImGui::SetCursorPosY(below);
    if (ctx.app.focus_warning)
    {
        widgets::notice("The menu couldn't take focus from the game: click it once so the game lets go of the mouse.",
                        true);
    }
    draw_status(ctx);
    draw_how_to(ctx);
}
} // namespace ui::pages
