#include "ui/theme.h"

namespace ui
{
void apply_theme(ImGuiStyle& style)
{
    using namespace theme;

    style.WindowPadding = ImVec2(0.0f, 0.0f); // the menu lays out its own header/sidebar/page areas
    style.FramePadding = ImVec2(10.0f, 7.0f);
    style.ItemSpacing = ImVec2(10.0f, 9.0f);
    style.ItemInnerSpacing = ImVec2(7.0f, 6.0f);
    style.CellPadding = ImVec2(8.0f, 5.0f);
    style.ScrollbarSize = 11.0f;
    style.GrabMinSize = 14.0f;

    style.WindowBorderSize = 1.0f;
    style.ChildBorderSize = 0.0f; // cards are set apart by their colour, not a border
    style.PopupBorderSize = 1.0f;
    style.FrameBorderSize = 0.0f;

    style.WindowRounding = 12.0f;
    style.ChildRounding = 12.0f; // cards
    style.FrameRounding = 8.0f;
    style.PopupRounding = 8.0f;
    style.ScrollbarRounding = 6.0f;
    style.GrabRounding = 8.0f;
    style.TabRounding = 8.0f;

    ImVec4* c = style.Colors;
    c[ImGuiCol_Text] = kText;
    c[ImGuiCol_TextDisabled] = kTextFaint;
    c[ImGuiCol_WindowBg] = kBackground;
    c[ImGuiCol_ChildBg] = rgb(0x000000, 0.0f);
    c[ImGuiCol_PopupBg] = kSurface;
    c[ImGuiCol_Border] = kBorder;
    c[ImGuiCol_BorderShadow] = rgb(0x000000, 0.0f);
    c[ImGuiCol_FrameBg] = kSurfaceHigh;
    c[ImGuiCol_FrameBgHovered] = kSurfaceHover;
    c[ImGuiCol_FrameBgActive] = kSurfaceHover;
    c[ImGuiCol_TitleBg] = kSidebar;
    c[ImGuiCol_TitleBgActive] = kSidebar;
    c[ImGuiCol_TitleBgCollapsed] = kSidebar;
    c[ImGuiCol_MenuBarBg] = kSidebar;
    c[ImGuiCol_ScrollbarBg] = rgb(0x000000, 0.0f);
    c[ImGuiCol_ScrollbarGrab] = kBorder;
    c[ImGuiCol_ScrollbarGrabHovered] = kSurfaceHover;
    c[ImGuiCol_ScrollbarGrabActive] = kAccent;
    c[ImGuiCol_CheckMark] = kAccent;
    c[ImGuiCol_SliderGrab] = kAccent;
    c[ImGuiCol_SliderGrabActive] = kAccentHover;
    c[ImGuiCol_Button] = kSurfaceHigh;
    c[ImGuiCol_ButtonHovered] = kSurfaceHover;
    c[ImGuiCol_ButtonActive] = kAccentSoft;
    c[ImGuiCol_Header] = kAccentSoft;
    c[ImGuiCol_HeaderHovered] = kSurfaceHover;
    c[ImGuiCol_HeaderActive] = kAccentSoft;
    c[ImGuiCol_Separator] = kBorder;
    c[ImGuiCol_SeparatorHovered] = kAccent;
    c[ImGuiCol_SeparatorActive] = kAccent;
    c[ImGuiCol_ResizeGrip] = rgb(0x000000, 0.0f);
    c[ImGuiCol_ResizeGripHovered] = kAccent;
    c[ImGuiCol_ResizeGripActive] = kAccentHover;
    c[ImGuiCol_TableHeaderBg] = kSurfaceHigh;
    c[ImGuiCol_TableRowBg] = rgb(0x000000, 0.0f);
    c[ImGuiCol_TableRowBgAlt] = rgb(0xFFFFFF, 0.03f);
    c[ImGuiCol_TableBorderLight] = kBorder;
    c[ImGuiCol_TableBorderStrong] = kBorder;
    c[ImGuiCol_TextSelectedBg] = rgb(0x8CC4CF, 0.35f);
    c[ImGuiCol_NavCursor] = kAccent;
}

float scaled(float pixels)
{
    return pixels * ImGui::GetStyle().FontScaleMain;
}
} // namespace ui
