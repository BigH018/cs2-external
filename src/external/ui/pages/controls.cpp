#include "ui/pages/controls.h"

#include <string>

#include "config.h"
#include "input/keys.h"
#include "ui/theme.h"
#include "ui/widgets.h"

namespace ui::pages
{
static_assert(sizeof(Color) == 4 * sizeof(float), "ImGui edits a Color as float[4]");

bool check(const char* label, bool* value, const char* help)
{
    const bool changed = ImGui::Checkbox(label, value);
    ImGui::SameLine();
    widgets::help_marker(help);
    return changed;
}

bool colour(const char* label, Color& value)
{
    return ImGui::ColorEdit4(label, &value.r, ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_NoInputs);
}

bool key_combo(const char* label, std::uint32_t& vk)
{
    const std::string current(input::key_name(vk));
    bool changed = false;
    if (ImGui::BeginCombo(label, current.c_str()))
    {
        for (const input::KeyInfo& key : input::kBindableKeys)
        {
            const std::string name(key.name);
            const bool selected = key.vk == vk;
            if (ImGui::Selectable(name.c_str(), selected))
            {
                vk = key.vk;
                changed = true;
            }
            if (selected)
            {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }
    return changed;
}

bool team_mode_combo(settings::TeamMode& mode)
{
    static constexpr const char* kTeamModes[] = {"Teams", "Free for all"};
    ImGui::SetNextItemWidth(scaled(180.0f));
    const bool changed = combo("Team mode", mode, kTeamModes, 2);
    ImGui::SameLine();
    widgets::help_marker("Who counts as an enemy, for every feature (ESP, aimbot, triggerbot, radar). Teams: the "
                         "other team. Free for all: everyone else (CS2 deathmatch is free for all).");
    return changed;
}

bool max_distance_slider(float& metres, const char* help)
{
    ImGui::SetNextItemWidth(scaled(240.0f));
    const bool changed =
        ImGui::SliderFloat("Max distance", &metres, config::kMaxDistance.min, config::kMaxDistance.max,
                           metres <= 0.0f ? "no limit" : "%.0f m", ImGuiSliderFlags_AlwaysClamp);
    ImGui::SameLine();
    widgets::help_marker(help);
    return changed;
}
} // namespace ui::pages
