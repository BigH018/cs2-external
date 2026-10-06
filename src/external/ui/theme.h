#pragma once

// The menu's colours and styling (dark navy, the same palette as the AC project). Every UI colour lives here; pages
// refer to these names, never to inline hex values. Phase 10 retunes the palette to the final logo.

#include <cstdint>

#include <imgui.h>

namespace ui
{
namespace theme
{
// 0xRRGGBB -> ImVec4 (ImGui colours are RGBA floats).
constexpr ImVec4 rgb(std::uint32_t hex, float alpha = 1.0f)
{
    return ImVec4(static_cast<float>((hex >> 16) & 0xFF) / 255.0f, static_cast<float>((hex >> 8) & 0xFF) / 255.0f,
                  static_cast<float>(hex & 0xFF) / 255.0f, alpha);
}

inline constexpr ImVec4 kBackground = rgb(0x1A1B26);   // page area (deep navy)
inline constexpr ImVec4 kSidebar = rgb(0x14151E);      // navigation column + header (darker navy)
inline constexpr ImVec4 kSurface = rgb(0x22243A);      // cards
inline constexpr ImVec4 kSurfaceHigh = rgb(0x2C2F49);  // inputs, buttons
inline constexpr ImVec4 kSurfaceHover = rgb(0x363A58); // hovered inputs/buttons
inline constexpr ImVec4 kBorder = rgb(0x3A3D5C);
inline constexpr ImVec4 kText = rgb(0xEDEBF7);         // lavender white
inline constexpr ImVec4 kTextDim = rgb(0x9DA0BC);
inline constexpr ImVec4 kTextFaint = rgb(0x6C6F8C);    // sidebar group labels, disabled text
inline constexpr ImVec4 kAccent = rgb(0x8CC4CF);       // soft teal-blue
inline constexpr ImVec4 kAccentHover = rgb(0xAEDAE2);
inline constexpr ImVec4 kAccentSoft = rgb(0x26384A);   // selected nav item / headers background
inline constexpr ImVec4 kLavender = rgb(0xC6BEEB);     // secondary highlight (the author line)
inline constexpr ImVec4 kOk = rgb(0x6FD3A0);
inline constexpr ImVec4 kWarn = rgb(0xF2C46B);
inline constexpr ImVec4 kDanger = rgb(0xF27272);
inline constexpr ImVec4 kWatermarkBg = rgb(0x14151E, 0.75f); // behind the watermark text on the game
} // namespace theme

// Fonts loaded once by ImGuiLayer. `bold` falls back to `regular` if the bold face isn't available.
struct Fonts
{
    ImFont* regular = nullptr;
    ImFont* bold = nullptr;
};

// Colours, rounding and spacing.
void apply_theme(ImGuiStyle& style);

// A size in pixels at UI scale 1.0. Phase 10 adds the menu size setting; until then this is the identity, but every
// layout size already goes through it.
float scaled(float pixels);
} // namespace ui
