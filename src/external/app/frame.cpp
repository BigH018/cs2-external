#include "app/frame.h"

#include <chrono>
#include <cstdint>
#include <optional>
#include <vector>

#include <imgui.h>

#include "app/state.h"
#include "config.h"
#include "core/log.h"
#include "core/runtime.h"
#include "features/activation.h"
#include "features/aimbot.h"
#include "features/bomb_timer.h"
#include "features/esp.h"
#include "features/radar.h"
#include "features/triggerbot.h"
#include "game/bomb.h"
#include "game/handle.h"
#include "game/offsets.h"
#include "game/player.h"
#include "game/writes.h"
#include "maths/vec.h"
#include "render/painter.h"
#include "render/primitives.h"
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

// Phase 5 polls the few keys it needs; Phase 7's keybind engine (input/key_poll) replaces this.
bool key_down(std::uint32_t vk) noexcept
{
    return (GetAsyncKeyState(static_cast<int>(vk)) & 0x8000) != 0;
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

        stop_features(); // let go of attack if the triggerbot holds it
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
        const settings::Settings& settings = state_.settings;
        state_.active.esp = settings.esp.enabled;
        state_.active.aimbot = settings.aimbot.enabled;
        state_.active.triggerbot = settings.triggerbot.enabled;
        state_.active.radar = settings.radar.enabled;
        state_.active.bomb_timer = settings.bomb_timer.enabled;
        read_game();

        // Aiming and firing only while you're playing: the game in front, the menu closed.
        const bool playing = game_focused && !state_.menu_open;
        const float frame_seconds = frame_time();
        run_aimbot(playing, frame_seconds);
        run_triggerbot(playing);

        imgui_.begin_frame();
        state_.fps = imgui_.framerate();
        draw_world();
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
        stop_features();
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

    // The game snapshot: every frame while a feature draws from it, otherwise at ~4 Hz for the Home page.
    void read_game()
    {
        const std::uint64_t now = GetTickCount64();
        const bool every_frame = state_.active.esp || state_.active.aimbot || state_.active.triggerbot ||
                                 state_.active.radar || state_.active.bomb_timer;
        if (!every_frame && now < next_status_ms_)
        {
            return;
        }
        next_status_ms_ = now + config::kStatusIntervalMs;
        const bool was_in_match = state_.in_match();
        const auto pawn = game::read_local_pawn(ctx_.memory, ctx_.client.base);
        state_.pawn_read_ok = pawn.has_value();
        state_.local_pawn = pawn.value_or(0);
        state_.snapshot = game::read_game(ctx_.memory, ctx_.client.base);
        if (state_.active.bomb_timer)
        {
            if (const auto entity_system = game::read_entity_system(ctx_.memory, ctx_.client.base))
            {
                state_.snapshot.bomb = game::read_bomb(ctx_.memory, ctx_.client.base, *entity_system);
            }
        }
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

    // Seconds since the previous frame (0 on the first), for the aimbot's frame-rate-independent smoothing.
    float frame_time()
    {
        const auto now = std::chrono::steady_clock::now();
        const float seconds = last_frame_ ? std::chrono::duration<float>(now - *last_frame_).count() : 0.0f;
        last_frame_ = now;
        return seconds;
    }

    // One smoothing step towards the target, written to the game's view angles.
    void run_aimbot(bool playing, float frame_seconds)
    {
        const settings::AimbotSettings& aim = state_.settings.aimbot;
        if (!aim.enabled)
        {
            aim_key_.reset();
            state_.aim_status = {};
            return;
        }
        const bool active = aim_key_.update(playing && key_down(aim.key), aim.mode) && playing;
        state_.aim_status = {active, false};
        if (!active)
        {
            return;
        }
        const auto angles =
            features::compute_aim(state_.snapshot, aim, state_.settings.general.team_mode, frame_seconds);
        state_.aim_status.has_target = angles.has_value();
        if (angles && !game::write_view_angles(ctx_.memory, ctx_.client.base, *angles))
        {
            warn_write_failed("view angles");
        }
    }

    void run_triggerbot(bool playing)
    {
        const settings::TriggerbotSettings& trigger = state_.settings.triggerbot;
        if (!trigger.enabled)
        {
            trigger_key_.reset();
            triggerbot_.reset();
            state_.trigger_status = {};
            set_attack(false);
            return;
        }
        bool allowed = true;
        if (trigger.activation != settings::TriggerActivation::always)
        {
            const settings::BindMode mode = trigger.activation == settings::TriggerActivation::hold
                                                ? settings::BindMode::hold
                                                : settings::BindMode::toggle;
            allowed = trigger_key_.update(playing && key_down(trigger.key), mode);
        }
        const bool active = allowed && playing;
        const features::TriggerBlock block =
            features::trigger_block(state_.snapshot, trigger, state_.settings.general.team_mode);
        state_.trigger_status = {active, block};
        set_attack(triggerbot_.update(GetTickCount64(), active, block == features::TriggerBlock::none, trigger));
    }

    // Presses or releases the game's attack button, only on a change. A release is skipped while you hold the
    // attack key yourself, so the triggerbot never cancels your own shooting.
    void set_attack(bool down)
    {
        if (down == attack_down_)
        {
            return;
        }
        attack_down_ = down;
        if (!down && key_down(VK_LBUTTON))
        {
            return;
        }
        if (!game::set_button(ctx_.memory, ctx_.client.base, game::offsets::buttons::attack, down))
        {
            warn_write_failed("attack button");
        }
    }

    // The game lost focus, the overlay hides or the tool exits: nothing may stay pressed.
    void stop_features()
    {
        triggerbot_.reset();
        set_attack(false);
        aim_key_.reset();
        trigger_key_.reset();
        last_frame_.reset();
    }

    void warn_write_failed(const char* what)
    {
        if (!write_warned_)
        {
            logger::warn("Writing the {} failed (is the handle missing write access?). Further failures aren't logged.",
                         what);
            write_warned_ = true;
        }
    }

    // Under the watermark and the menu, on ImGui's background draw list: the ESP, the aimbot's FOV circle, the radar
    // and the bomb timer.
    void draw_world()
    {
        const maths::Vec2 screen{static_cast<float>(state_.overlay_width), static_cast<float>(state_.overlay_height)};
        const float font_size = ui::scaled(config::kEspFontSize);
        std::vector<render::Primitive> primitives = features::build_esp(
            state_.snapshot, state_.settings.esp, state_.settings.general.team_mode, screen, font_size);
        const settings::AimbotSettings& aim = state_.settings.aimbot;
        if (aim.enabled && aim.draw_fov)
        {
            if (const auto circle = features::fov_circle(state_.snapshot, aim, screen))
            {
                primitives.push_back(*circle);
            }
        }
        const std::vector<render::Primitive> radar =
            features::build_radar(state_.snapshot, state_.settings.radar, state_.settings.general.team_mode, screen);
        primitives.insert(primitives.end(), radar.begin(), radar.end());
        const std::vector<render::Primitive> bomb =
            features::build_bomb_timer(state_.snapshot, state_.settings.bomb_timer, screen, font_size);
        primitives.insert(primitives.end(), bomb.begin(), bomb.end());
        render::paint(*ImGui::GetBackgroundDrawList(), primitives, imgui_.fonts().regular, font_size);
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
    features::KeyActivation aim_key_;
    features::KeyActivation trigger_key_;
    features::Triggerbot triggerbot_;
    bool attack_down_ = false; // what we last wrote to the attack button
    bool write_warned_ = false;
    std::optional<std::chrono::steady_clock::time_point> last_frame_;
};
} // namespace

int run(const Context& ctx)
{
    Runner runner(ctx);
    return runner.run();
}
} // namespace app
