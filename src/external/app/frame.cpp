#include "app/frame.h"

#include <cstdint>

#include "app/state.h"
#include "config.h"
#include "core/log.h"
#include "core/runtime.h"
#include "game/player.h"
#include "ui/hud.h"
#include "ui/imgui_layer.h"
#include "ui/menu.h"
#include "ui/overlay_window.h"

namespace app
{
namespace
{
// The game window's client area in screen coordinates. false if it has none (minimised, zero-sized).
bool client_area(HWND window, RECT& area) noexcept
{
    RECT client{};
    POINT top_left{0, 0};
    if (!GetClientRect(window, &client) || !ClientToScreen(window, &top_left))
    {
        return false;
    }
    area = RECT{top_left.x, top_left.y, top_left.x + client.right, top_left.y + client.bottom};
    return client.right > 0 && client.bottom > 0;
}

class Runner
{
public:
    explicit Runner(const Context& ctx) : ctx_(ctx)
    {
        state_.game = GameInfo{ctx.pid, ctx.client, ctx.engine};
        state_.offsets = ctx.offsets;
    }

    int run()
    {
        if (!overlay_.create(&ui::imgui_message_hook) ||
            !imgui_.init(overlay_.hwnd(), overlay_.device(), overlay_.context()))
        {
            return 1;
        }
        logger::info("Overlay running. {} opens the menu. Exit: Ctrl+C here (or close this window), or Alt+F4 while "
                     "the menu is open.",
                     config::kMenuToggleKeyName);

        int exit_code = 0;
        while (!core::shutdown_requested.load())
        {
            const ui::OverlayWindow::Events events = overlay_.pump();
            if (events.close_requested)
            {
                logger::info("Overlay closed: exiting");
                break;
            }
            if (!core::is_running(ctx_.process))
            {
                logger::info("cs2.exe has closed: exiting");
                break;
            }
            if (!frame(events))
            {
                exit_code = 1;
                break;
            }
        }

        if (state_.menu_open)
        {
            close_menu(true);
        }
        imgui_.shutdown(); // before the overlay releases the D3D device ImGui's objects live on
        overlay_.destroy();
        logger::info("Overlay removed");
        return exit_code;
    }

private:
    // One frame. false = the overlay can't go on (device lost).
    bool frame(const ui::OverlayWindow::Events& events)
    {
        if (!find_game_window())
        {
            hide();
            return true;
        }

        const HWND foreground = GetForegroundWindow();
        const bool game_focused = foreground == game_window_;
        const bool overlay_focused = foreground == overlay_.hwnd();
        overlay_.enable_menu_hotkey(game_focused || overlay_focused);

        if (events.menu_key && state_.menu_open)
        {
            close_menu(true);
        }
        else if (events.menu_key)
        {
            open_menu();
        }
        else if (state_.menu_open && overlay_had_focus_ && !overlay_focused)
        {
            close_menu(false); // focus moved on (Alt+Tab, a click on the game's title bar): leave it there
        }
        overlay_had_focus_ = GetForegroundWindow() == overlay_.hwnd();

        RECT area{};
        const bool ours = game_focused || overlay_focused || state_.menu_open;
        if (!ours || IsIconic(game_window_) || !client_area(game_window_, area))
        {
            hide();
            return true;
        }
        follow(area);
        read_status();

        imgui_.begin_frame();
        state_.fps = imgui_.framerate();
        ui::draw_hud(imgui_.fonts(), imgui_.logo(), state_);
        if (state_.menu_open)
        {
            ui::draw_menu(menu_, imgui_.fonts(), imgui_.logo(), state_);
        }
        overlay_.begin_frame();
        imgui_.end_frame();
        return overlay_.present();
    }

    bool find_game_window()
    {
        if (game_window_ != nullptr && IsWindow(game_window_))
        {
            return true;
        }
        if (game_window_ != nullptr)
        {
            logger::warn("The game window went away; waiting for a new one");
            game_window_ = nullptr;
            if (state_.menu_open)
            {
                close_menu(false);
            }
        }
        game_window_ = core::find_main_window(ctx_.pid);
        if (game_window_ != nullptr)
        {
            logger::info("Game window 0x{:X}", reinterpret_cast<std::uintptr_t>(game_window_));
            waiting_logged_ = false;
        }
        else if (!waiting_logged_)
        {
            logger::info("Waiting for the game window...");
            waiting_logged_ = true;
        }
        return game_window_ != nullptr;
    }

    // The game lost focus or is minimised: nothing on screen, and nothing to do but wait.
    void hide()
    {
        if (state_.menu_open)
        {
            close_menu(false);
        }
        overlay_.set_visible(false);
        overlay_.enable_menu_hotkey(false);
        overlay_.wait(config::kHiddenPollIntervalMs);
    }

    void follow(const RECT& area)
    {
        if (!EqualRect(&area, &last_area_))
        {
            last_area_ = area;
            state_.overlay_width = area.right - area.left;
            state_.overlay_height = area.bottom - area.top;
            logger::info("Overlay covers {}x{} at ({}, {})", state_.overlay_width, state_.overlay_height, area.left,
                         area.top);
        }
        overlay_.cover(area);
        overlay_.set_visible(true);
    }

    void read_status()
    {
        const std::uint64_t now = GetTickCount64();
        if (now < next_status_ms_)
        {
            return;
        }
        next_status_ms_ = now + config::kStatusIntervalMs;
        const bool was_in_match = state_.in_match();
        const auto pawn = game::read_local_pawn(ctx_.memory, ctx_.client.base);
        state_.pawn_read_ok = pawn.has_value();
        state_.local_pawn = pawn.value_or(0);
        if (state_.in_match() != was_in_match)
        {
            if (state_.in_match())
            {
                logger::info("In a match (local pawn 0x{:X})", state_.local_pawn);
            }
            else
            {
                logger::info("Not in a match");
            }
        }
    }

    void open_menu()
    {
        state_.menu_open = true;
        overlay_.set_interactive(true);
        imgui_.set_menu_open(true);
        // Taking focus is what makes the game let go of the mouse: the cursor shows, and clicks and keys come to the
        // menu instead of the game. Allowed because the menu hotkey was input to our process.
        state_.focus_warning = SetForegroundWindow(overlay_.hwnd()) == FALSE;
        if (state_.focus_warning)
        {
            logger::warn("The menu couldn't take focus from the game: click it once");
        }
    }

    void close_menu(bool give_focus_back)
    {
        state_.menu_open = false;
        state_.focus_warning = false;
        imgui_.set_menu_open(false);
        overlay_.set_interactive(false);
        if (give_focus_back && game_window_ != nullptr && IsWindow(game_window_))
        {
            SetForegroundWindow(game_window_);
        }
    }

    const Context& ctx_;
    ui::OverlayWindow overlay_;
    ui::ImGuiLayer imgui_;
    AppState state_;
    ui::MenuState menu_;
    HWND game_window_ = nullptr;
    bool waiting_logged_ = false;
    bool overlay_had_focus_ = false;
    RECT last_area_{};
    std::uint64_t next_status_ms_ = 0;
};
} // namespace

int run(const Context& ctx)
{
    Runner runner(ctx);
    return runner.run();
}
} // namespace app
