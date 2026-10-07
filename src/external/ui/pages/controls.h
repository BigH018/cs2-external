#pragma once

// Controls the feature pages share: a checkbox with a (?), colour pickers, enum combos and the shared team mode
// (keys: ui/keybind_widgets). Each returns true when the user changed the value.

#include <imgui.h>

#include "color.h"
#include "settings/settings.h"

namespace ui::pages
{
// A checkbox with a (?) after it.
bool check(const char* label, bool* value, const char* help);

// A colour swatch with an alpha bar.
bool colour(const char* label, Color& value);

// An enum as a combo; `names` lists the enumerators in order.
template <class Enum>
bool combo(const char* label, Enum& value, const char* const* names, int count)
{
    int index = static_cast<int>(value);
    if (ImGui::Combo(label, &index, names, count))
    {
        value = static_cast<Enum>(index);
        return true;
    }
    return false;
}

// The team mode every feature shares (settings::GeneralSettings), with its (?) explaining it.
bool team_mode_combo(settings::TeamMode& mode);

// A max-distance slider in metres; all the way left = no limit.
bool max_distance_slider(float& metres, const char* help);
} // namespace ui::pages
