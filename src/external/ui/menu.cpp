#include "ui/menu.h"

#include <algorithm>
#include <array>

#include <imgui.h>

#include "config.h"
#include "ui/pages/pages.h"
#include "ui/widgets.h"

namespace ui
{
namespace
{
struct PageEntry
{
    Page page;
    const char* label;
    const char* group; // printed above the first entry of each group (nullptr = no heading)
    void (*draw)(pages::PageContext&);
};

constexpr std::array kPages = {
    PageEntry{Page::Home, "Home", nullptr, &pages::draw_home},
    PageEntry{Page::Aimbot, "Aimbot", "COMBAT", &pages::draw_aimbot},
    PageEntry{Page::Triggerbot, "Triggerbot", nullptr, &pages::draw_triggerbot},
    PageEntry{Page::Esp, "ESP", "VISUALS", &pages::draw_esp},
    PageEntry{Page::Misc, "Misc", nullptr, &pages::draw_misc},
    PageEntry{Page::Keybinds, "Keybinds", "SETUP", &pages::draw_keybinds},
    PageEntry{Page::Settings, "Settings", nullptr, &pages::draw_settings},
};

// Sizes at UI scale 1.0.
constexpr float kAreaPadding = 20.0f;
constexpr float kNavItemHeight = 36.0f;
constexpr float kNavIndent = 16.0f;
constexpr float kNavAccentBar = 3.0f;
constexpr float kGroupFontSize = config::kFontSize - 4.0f;

ImU32 colour(const ImVec4& c)
{
    return ImGui::GetColorU32(c);
}

// The header and sidebar backgrounds are painted on the window itself, with the window's rounded corners, so nothing
// square pokes out of the rounded window.
void paint_backgrounds()
{
    ImDrawList* draw = ImGui::GetWindowDrawList();
    const ImVec2 pos = ImGui::GetWindowPos();
    const ImVec2 size = ImGui::GetWindowSize();
    const float rounding = ImGui::GetStyle().WindowRounding;
    const float header = scaled(config::kHeaderHeight);
    draw->AddRectFilled(pos, ImVec2(pos.x + size.x, pos.y + header), colour(theme::kSidebar), rounding,
                        ImDrawFlags_RoundCornersTop);
    draw->AddRectFilled(ImVec2(pos.x, pos.y + header), ImVec2(pos.x + scaled(config::kSidebarWidth), pos.y + size.y),
                        colour(theme::kSidebar), rounding, ImDrawFlags_RoundCornersBottomLeft);
    draw->AddLine(ImVec2(pos.x, pos.y + header), ImVec2(pos.x + size.x, pos.y + header), colour(theme::kBorder));
}

void draw_header(const Fonts& fonts, ImTextureData* logo)
{
    const float header = scaled(config::kHeaderHeight);
    const float padding = scaled(kAreaPadding);
    const float logo_size = scaled(config::kHeaderLogoSize);
    const ImVec2 origin = ImGui::GetCursorPos();

    // Logo, title, author.
    ImGui::SetCursorPos(ImVec2(origin.x + padding * 0.6f, origin.y + (header - logo_size) * 0.5f));
    widgets::image_rounded(logo, logo_size, logo_size * 0.25f);
    ImGui::SameLine(0.0f, scaled(12.0f));
    ImGui::PushFont(fonts.bold, scaled(config::kTitleFontSize));
    const float title_height = ImGui::GetTextLineHeight();
    ImGui::SetCursorPosY(origin.y + (header - title_height) * 0.5f);
    ImGui::TextUnformatted(config::kAppName);
    ImGui::PopFont();
    const float small_y = origin.y + (header - title_height) * 0.5f + (title_height - ImGui::GetTextLineHeight()) * 0.8f;
    ImGui::SameLine(0.0f, scaled(8.0f));
    ImGui::SetCursorPosY(small_y);
    ImGui::TextColored(theme::kLavender, "%s", config::kAppAuthor);

    // The edition pill on the right.
    const float pill_width = ImGui::CalcTextSize(config::kAppEdition).x + 2.0f * scaled(9.0f);
    const float pill_y = origin.y + (header - (ImGui::GetTextLineHeight() + 2.0f * scaled(3.0f))) * 0.5f;
    ImGui::SameLine();
    ImGui::SetCursorPos(
        ImVec2(std::max(ImGui::GetCursorPosX(), ImGui::GetWindowWidth() - pill_width - padding), pill_y));
    widgets::pill(config::kAppEdition, theme::kAccent);

    ImGui::SetCursorPos(ImVec2(origin.x, origin.y + header));
}

void draw_sidebar(MenuState& state)
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(scaled(10.0f), scaled(12.0f)));
    ImGui::BeginChild("##sidebar", ImVec2(scaled(config::kSidebarWidth), 0.0f), ImGuiChildFlags_AlwaysUseWindowPadding);
    ImDrawList* draw = ImGui::GetWindowDrawList();
    for (const PageEntry& entry : kPages)
    {
        if (entry.group != nullptr)
        {
            ImGui::Dummy(ImVec2(0.0f, scaled(4.0f)));
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + scaled(6.0f));
            ImGui::PushFont(nullptr, scaled(kGroupFontSize));
            ImGui::TextColored(theme::kTextFaint, "%s", entry.group);
            ImGui::PopFont();
        }
        const bool selected = state.page == entry.page;
        const ImVec2 min = ImGui::GetCursorScreenPos();
        const ImVec2 size(ImGui::GetContentRegionAvail().x, scaled(kNavItemHeight));
        ImGui::PushID(static_cast<int>(entry.page));
        if (ImGui::InvisibleButton("##nav", size))
        {
            state.page = entry.page;
        }
        const bool hovered = ImGui::IsItemHovered();
        ImGui::PopID();
        const ImVec2 max(min.x + size.x, min.y + size.y);
        if (selected || hovered)
        {
            draw->AddRectFilled(min, max, colour(selected ? theme::kAccentSoft : theme::kSurface), scaled(8.0f));
        }
        if (selected)
        {
            draw->AddRectFilled(ImVec2(min.x, min.y + scaled(8.0f)),
                                ImVec2(min.x + scaled(kNavAccentBar), max.y - scaled(8.0f)), colour(theme::kAccent),
                                scaled(2.0f));
        }
        const ImU32 text = colour(selected ? theme::kAccent : (hovered ? theme::kText : theme::kTextDim));
        const ImVec2 label_size = ImGui::CalcTextSize(entry.label);
        draw->AddText(ImVec2(min.x + scaled(kNavIndent), min.y + (size.y - label_size.y) * 0.5f), text, entry.label);
    }
    ImGui::EndChild();
    ImGui::PopStyleVar();
}

void draw_page(MenuState& state, pages::PageContext& ctx)
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(scaled(kAreaPadding), scaled(kAreaPadding) * 0.8f));
    ImGui::BeginChild("##page", ImVec2(0.0f, 0.0f), ImGuiChildFlags_AlwaysUseWindowPadding);
    for (const PageEntry& entry : kPages)
    {
        if (entry.page == state.page)
        {
            entry.draw(ctx);
        }
    }
    ImGui::EndChild();
    ImGui::PopStyleVar();
}
} // namespace

void draw_menu(MenuState& state, const Fonts& fonts, ImTextureData* logo, app::AppState& app)
{
    // First appearance: centred, at the configured size but never bigger than the game window. Movable, resizable.
    const ImVec2 display = ImGui::GetIO().DisplaySize;
    const ImVec2 size(std::min(scaled(config::kMenuWidth), display.x * config::kMenuMaxScreenFraction),
                      std::min(scaled(config::kMenuHeight), display.y * config::kMenuMaxScreenFraction));
    ImGui::SetNextWindowSize(size, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowPos(ImVec2(display.x * 0.5f, display.y * 0.5f), ImGuiCond_FirstUseEver, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSizeConstraints(
        ImVec2(std::min(scaled(config::kMenuMinWidth), display.x), std::min(scaled(config::kMenuMinHeight), display.y)),
        ImVec2(display.x, display.y));

    constexpr ImGuiWindowFlags kFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
                                        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
    if (ImGui::Begin("External Cheat##menu", nullptr, kFlags))
    {
        paint_backgrounds();
        draw_header(fonts, logo);
        draw_sidebar(state);
        ImGui::SameLine(0.0f, 0.0f);
        pages::PageContext ctx{fonts, app, logo, state};
        draw_page(state, ctx);
    }
    ImGui::End();
}
} // namespace ui
