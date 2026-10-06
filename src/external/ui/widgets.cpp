#include "ui/widgets.h"

#include <algorithm>

#include "config.h"

namespace ui::widgets
{
namespace
{
// Sizes at UI scale 1.0.
constexpr float kCardPadding = 16.0f;
constexpr float kCardSpacing = 12.0f;
constexpr float kTooltipWidthInFonts = 30.0f; // tooltip text wraps at this many font sizes
constexpr float kPillPaddingX = 9.0f;
constexpr float kPillPaddingY = 3.0f;
constexpr float kPillFillAlpha = 0.16f;
constexpr float kLabelColumnFraction = 0.4f; // the row-label column never takes more of a card than this

// Where the value of a "label: value" row starts (window-relative x). Call at the start of the line.
float label_column_end(const char* label)
{
    const float available = ImGui::GetContentRegionAvail().x;
    const float column = std::min(scaled(config::kRowLabelWidth), available * kLabelColumnFraction);
    const float needed = ImGui::CalcTextSize(label, nullptr, true).x + ImGui::GetStyle().ItemSpacing.x;
    return ImGui::GetCursorPosX() + std::max(column, needed);
}
} // namespace

void page_header(const Fonts& fonts, const char* title, const char* subtitle)
{
    ImGui::PushFont(fonts.bold, scaled(config::kPageTitleFontSize));
    ImGui::TextUnformatted(title);
    ImGui::PopFont();
    ImGui::PushStyleColor(ImGuiCol_Text, theme::kTextDim);
    ImGui::TextWrapped("%s", subtitle);
    ImGui::PopStyleColor();
    ImGui::Dummy(ImVec2(0.0f, scaled(4.0f)));
}

void card_begin(const Fonts& fonts, const char* title)
{
    ImGui::PushStyleColor(ImGuiCol_ChildBg, theme::kSurface);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(scaled(kCardPadding), scaled(kCardPadding)));
    ImGui::BeginChild(title, ImVec2(0.0f, 0.0f), ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_AlwaysUseWindowPadding);
    ImGui::PushFont(fonts.bold, scaled(config::kFontSize + 1.0f));
    ImGui::TextUnformatted(title);
    ImGui::PopFont();
    ImGui::Dummy(ImVec2(0.0f, scaled(2.0f)));
}

void card_end()
{
    ImGui::EndChild();
    ImGui::PopStyleVar();
    ImGui::PopStyleColor();
    ImGui::Dummy(ImVec2(0.0f, scaled(kCardSpacing) - ImGui::GetStyle().ItemSpacing.y));
}

void hint(const char* text)
{
    ImGui::PushStyleColor(ImGuiCol_Text, theme::kTextDim);
    ImGui::TextWrapped("%s", text);
    ImGui::PopStyleColor();
}

void help_marker(const char* text)
{
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered() && ImGui::BeginTooltip())
    {
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * kTooltipWidthInFonts);
        ImGui::TextUnformatted(text);
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}

void info_row(const char* label, const char* value)
{
    info_row(label, value, theme::kText);
}

void info_row(const char* label, const char* value, const ImVec4& value_colour)
{
    const float value_x = label_column_end(label);
    ImGui::TextColored(theme::kTextDim, "%s", label);
    ImGui::SameLine(value_x);
    ImGui::TextColored(value_colour, "%s", value);
}

void notice(const char* text, bool warning)
{
    ImGui::PushStyleColor(ImGuiCol_Text, warning ? theme::kWarn : theme::kAccent);
    ImGui::TextWrapped("%s", text);
    ImGui::PopStyleColor();
}

void pill(const char* text, const ImVec4& colour)
{
    const ImVec2 padding(scaled(kPillPaddingX), scaled(kPillPaddingY));
    const ImVec2 text_size = ImGui::CalcTextSize(text);
    const ImVec2 size(text_size.x + 2.0f * padding.x, text_size.y + 2.0f * padding.y);
    const ImVec2 min = ImGui::GetCursorScreenPos();
    const ImVec2 max(min.x + size.x, min.y + size.y);
    ImVec4 fill = colour;
    fill.w = kPillFillAlpha;
    ImDrawList* draw = ImGui::GetWindowDrawList();
    draw->AddRectFilled(min, max, ImGui::GetColorU32(fill), size.y * 0.5f);
    draw->AddRect(min, max, ImGui::GetColorU32(colour), size.y * 0.5f);
    draw->AddText(ImVec2(min.x + padding.x, min.y + padding.y), ImGui::GetColorU32(colour), text);
    ImGui::Dummy(size);
}

void planned_card(const Fonts& fonts, const char* phase, std::initializer_list<const char*> features)
{
    card_begin(fonts, "Planned");
    ImGui::TextColored(theme::kLavender, "Arrives in %s.", phase);
    for (const char* feature : features)
    {
        ImGui::Bullet();
        ImGui::TextColored(theme::kTextDim, "%s", feature);
    }
    card_end();
}

void image_rounded(ImTextureData* texture, float size, float rounding)
{
    const ImVec2 min = ImGui::GetCursorScreenPos();
    ImGui::Dummy(ImVec2(size, size));
    if (texture == nullptr)
    {
        return;
    }
    ImGui::GetWindowDrawList()->AddImageRounded(texture->GetTexRef(), min, ImVec2(min.x + size, min.y + size),
                                                ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f), IM_COL32_WHITE, rounding);
}
} // namespace ui::widgets
