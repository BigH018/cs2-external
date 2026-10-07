#pragma once

// The menu window: a header (title, author, status pills), a grouped sidebar and the current page. Drawn only while
// the menu is open. The menu never touches game memory: it edits app::AppState and app/frame applies it.

#include <array>
#include <string>

#include <imgui.h>

#include "app/state.h"
#include "ui/theme.h"

namespace ui
{
enum class Page
{
    Home,
    Aimbot,
    Triggerbot,
    Esp,
    Misc,
    Keybinds,
    Settings,
};

// UI-side state that survives between frames (which page is selected). Main thread only.
struct MenuState
{
    Page page = Page::Home;

    // Settings page, Profiles card.
    std::string selected_profile;        // in the list (empty = the current profile)
    std::string confirm_delete;          // "Really delete?" is showing for this profile
    std::array<char, 64> profile_name{}; // the name box (Save as, Rename)
};

// `logo` may be null (then the logo spots stay empty).
void draw_menu(MenuState& state, const Fonts& fonts, ImTextureData* logo, app::AppState& app);
} // namespace ui
