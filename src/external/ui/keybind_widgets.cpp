#include "ui/keybind_widgets.h"

#include <span>
#include <string>

#include <imgui.h>

#include "input/keybinds.h"
#include "input/keys.h"
#include "ui/theme.h"
#include "ui/widgets.h"

namespace ui::keybind
{
namespace
{
// Sizes at UI scale 1.0.
constexpr float kKeyButtonWidth = 130.0f;
constexpr float kModeWidth = 100.0f;
constexpr float kRowLabelWidth = 130.0f;

constexpr char kCaptureText[] = "Press a key...";
constexpr char kHelp[] = "Click, then press a key or mouse button (for Mouse 1, click outside the menu). Esc clears "
                         "the bind, clicking again cancels.";
constexpr char kMenuKeyHelp[] = "Click, then press a keyboard key. The menu key is a Windows hotkey, so it can't be a "
                                "mouse button or Shift / Ctrl / Alt on its own, and it can't be cleared. Esc or "
                                "clicking again cancels.";

// The other actions bound to this action's key, as one line ("ESP on / off, Radar on / off"), or empty.
std::string conflicting_with(const app::AppState& app, input::ActionId id)
{
    const std::uint32_t key = app.settings.keybinds.bind(id).key;
    std::string others;
    for (const input::Conflict& conflict : input::find_conflicts(app.settings.keybinds.binds))
    {
        if (conflict.key != key)
        {
            continue;
        }
        for (const input::ActionId other : conflict.actions)
        {
            if (other != id)
            {
                others += (others.empty() ? "" : ", ") + std::string(input::action(other).label);
            }
        }
    }
    return others;
}
} // namespace

void key_button(app::AppState& app, input::ActionId id)
{
    const input::Bind& bind = app.settings.keybinds.bind(id);
    const bool capturing = app.capture.capturing(id);
    const std::string others = capturing ? std::string() : conflicting_with(app, id);
    const std::string label = (capturing ? std::string(kCaptureText) : input::key_name(bind.key)) + "##key";

    ImGui::PushID(static_cast<int>(input::index_of(id)));
    int colours = 0;
    if (capturing)
    {
        ImGui::PushStyleColor(ImGuiCol_Button, theme::kAccentSoft);
        ImGui::PushStyleColor(ImGuiCol_Text, theme::kAccent);
        ImGui::PushStyleColor(ImGuiCol_Border, theme::kAccent);
        colours = 3;
    }
    else if (!others.empty())
    {
        ImGui::PushStyleColor(ImGuiCol_Text, theme::kDanger);
        ImGui::PushStyleColor(ImGuiCol_Border, theme::kDanger);
        colours = 2;
    }
    else if (bind.key == input::kUnbound)
    {
        ImGui::PushStyleColor(ImGuiCol_Text, theme::kTextDim);
        colours = 1;
    }
    if (ImGui::Button(label.c_str(), ImVec2(scaled(kKeyButtonWidth), 0.0f)))
    {
        if (capturing)
        {
            app.capture.cancel();
        }
        else
        {
            app.capture.start(id, app.now_ms);
        }
    }
    ImGui::PopStyleColor(colours);
    if (ImGui::IsItemHovered())
    {
        if (others.empty())
        {
            ImGui::SetTooltip("%s", input::action(id).hotkey ? kMenuKeyHelp : kHelp);
        }
        else
        {
            ImGui::SetTooltip("%s is also bound to: %s", input::key_name(bind.key).c_str(), others.c_str());
        }
    }
    ImGui::PopID();
}

void mode_selector(app::AppState& app, input::ActionId id)
{
    const std::span<const input::BindMode> modes = input::allowed_modes(id);
    if (modes.size() < 2)
    {
        return;
    }
    input::Bind& bind = app.settings.keybinds.bind(id);
    ImGui::PushID(static_cast<int>(input::index_of(id)));
    ImGui::SetNextItemWidth(scaled(kModeWidth));
    const std::string current(input::mode_name(bind.mode));
    if (ImGui::BeginCombo("##mode", current.c_str()))
    {
        for (const input::BindMode mode : modes)
        {
            const std::string name(input::mode_name(mode));
            if (ImGui::Selectable(name.c_str(), mode == bind.mode))
            {
                bind.mode = mode;
            }
        }
        ImGui::EndCombo();
    }
    ImGui::PopID();
}

void bind_row(app::AppState& app, input::ActionId id, const char* label)
{
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted(label);
    ImGui::SameLine(scaled(kRowLabelWidth) + ImGui::GetStyle().WindowPadding.x);
    key_button(app, id);
    if (input::allowed_modes(id).size() > 1)
    {
        ImGui::SameLine();
        mode_selector(app, id);
        ImGui::SameLine();
        widgets::help_marker("Hold: on while the key is down. Toggle: each press switches it on or off.");
    }
}
} // namespace ui::keybind
