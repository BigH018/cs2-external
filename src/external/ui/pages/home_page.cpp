#include <format>
#include <string>

#include <imgui.h>

#include "config.h"
#include "features/feature_summary.h"
#include "game/weapon.h"
#include "input/keys.h"
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

// Map, players and the local player, from the ~4 Hz snapshot.
void draw_match(const game::GameSnapshot& snapshot)
{
    if (!snapshot.in_match)
    {
        return;
    }
    widgets::info_row("Map", snapshot.globals.map_name.empty() ? "?" : snapshot.globals.map_name.c_str());
    int alive = 0;
    for (const game::PlayerSnapshot& player : snapshot.players)
    {
        alive += player.alive ? 1 : 0;
    }
    widgets::info_row("Players", std::format("{} in the match, {} alive", snapshot.players.size(), alive).c_str());
    if (const game::PlayerSnapshot* local = snapshot.local(); local != nullptr)
    {
        std::string you = std::format("{}  ({})", local->name, game::team_short_name(local->team));
        if (local->alive)
        {
            const std::string_view weapon =
                local->weapon_id ? game::weapon_info(*local->weapon_id).name : std::string_view{};
            you += std::format("  {} HP  {}", local->health, weapon.empty() ? "-" : weapon);
        }
        else
        {
            you += "  dead";
        }
        widgets::info_row("You", you.c_str());
    }
}

void draw_status(PageContext& ctx)
{
    const app::AppState& app = ctx.app;
    widgets::card_begin(ctx.fonts, "Status");
    widgets::info_row("Game", std::format("cs2.exe  (PID {})", app.game.pid).c_str());
    widgets::info_row("client.dll", module_text(app.game.client).c_str());
    widgets::info_row("engine2.dll", module_text(app.game.engine).c_str());
    const app::OffsetReport& offsets = app.offsets;
    if (offsets.ok())
    {
        widgets::info_row("Offsets",
                          std::format("build {}: all {} checks OK", offsets.game_build, offsets.checks).c_str(),
                          theme::kOk);
    }
    else if (offsets.ran)
    {
        widgets::info_row("Offsets",
                          std::format("build {}: {} of {} checks failed (see the console)", offsets.game_build,
                                      offsets.failures, offsets.checks)
                              .c_str(),
                          theme::kWarn);
    }
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
        draw_match(app.snapshot);
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
    const settings::KeybindSettings& keys = ctx.app.settings.keybinds;
    widgets::hint(std::format("{} opens and closes this menu. While it's open the game doesn't get your mouse or "
                              "keyboard, so clicks here never shoot.",
                              input::key_name(keys.bind(input::ActionId::menu_toggle).key))
                      .c_str());
    widgets::hint("The overlay is a separate, click-through window on top of the game. It hides when the game loses "
                  "focus (Alt+Tab) and follows the game window when it moves or changes resolution.");
    widgets::hint(std::format("Panic ({}) turns every feature off. To exit: {}, Ctrl+C in the console window (or close "
                              "it), or Alt+F4 while the menu is open. The game keeps running. Every key can be changed "
                              "on the Keybinds page.",
                              input::key_name(keys.bind(input::ActionId::panic).key),
                              input::key_name(keys.bind(input::ActionId::exit).key))
                      .c_str());
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
