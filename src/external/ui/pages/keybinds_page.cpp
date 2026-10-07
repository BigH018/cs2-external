#include <string>
#include <vector>

#include <imgui.h>

#include "input/actions.h"
#include "input/keybinds.h"
#include "input/keys.h"
#include "ui/keybind_widgets.h"
#include "ui/pages/pages.h"
#include "ui/widgets.h"

// Keybinds page: every action from the registry (input/actions), grouped by category, with its key, its mode and a
// warning for keys bound to several actions. Built from the registry, so a new action shows up here with no UI work.

namespace ui::pages
{
namespace
{
constexpr float kLabelColumnWidth = 260.0f;

void draw_conflicts(const app::AppState& app)
{
    const std::vector<input::Conflict> conflicts = input::find_conflicts(app.settings.keybinds.binds);
    if (conflicts.empty())
    {
        return;
    }
    std::string text = "One key, several actions (each of them fires):";
    for (const input::Conflict& conflict : conflicts)
    {
        text += "\n" + input::key_name(conflict.key) + ": ";
        for (std::size_t i = 0; i < conflict.actions.size(); ++i)
        {
            text += (i == 0 ? "" : ", ") + std::string(input::action(conflict.actions[i]).label);
        }
    }
    widgets::notice(text.c_str(), true);
    ImGui::Spacing();
}

void draw_category(PageContext& ctx, input::Category category)
{
    const std::string title(input::category_name(category));
    widgets::card_begin(ctx.fonts, title.c_str());
    if (ImGui::BeginTable("##binds", 3, ImGuiTableFlags_SizingFixedFit))
    {
        ImGui::TableSetupColumn("action", ImGuiTableColumnFlags_WidthFixed, scaled(kLabelColumnWidth));
        for (const input::ActionDef& def : input::actions())
        {
            if (def.category != category)
            {
                continue;
            }
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted(def.label.data(), def.label.data() + def.label.size());
            ImGui::TableSetColumnIndex(1);
            keybind::key_button(ctx.app, def.id);
            ImGui::TableSetColumnIndex(2);
            keybind::mode_selector(ctx.app, def.id);
        }
        ImGui::EndTable();
    }
    widgets::card_end();
}
} // namespace

void draw_keybinds(PageContext& ctx)
{
    widgets::page_header(ctx.fonts, "Keybinds",
                         "Every action you can put on a key or mouse button. Keys work while the game is in front.");
    app::AppState& app = ctx.app;
    if (app.menu_key_failed)
    {
        widgets::notice("The menu key couldn't be registered: another program (or Windows) uses it as a hotkey. Pick "
                        "another one below.",
                        true);
        ImGui::Spacing();
    }
    widgets::hint("Click a key button, then press any key or mouse button (Esc clears it). For Mouse 1, click "
                  "outside the menu.");
    ImGui::SameLine();
    widgets::help_marker("Hold = on while held, Toggle = each press switches it, the rest fire once per press. While "
                         "this menu is open only the menu key, panic and exit work. Clicking a key button again "
                         "cancels; after 6 seconds it gives up.");
    ImGui::Spacing();
    draw_conflicts(app);
    for (const input::Category category : input::kCategories)
    {
        draw_category(ctx, category);
    }
    if (ImGui::Button("Reset every keybind"))
    {
        app.capture.cancel();
        app.settings.keybinds = settings::KeybindSettings{};
    }
    ImGui::SameLine();
    widgets::help_marker("Back to the defaults: INSERT menu, END panic, DELETE exit, Mouse 1 aim (hold), Mouse 4 "
                         "trigger (hold), every on / off key unbound.");
}
} // namespace ui::pages
