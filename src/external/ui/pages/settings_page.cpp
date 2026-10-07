#include <algorithm>
#include <string>
#include <utility>
#include <vector>

#include <imgui.h>

#include "config.h"
#include "settings/presets.h"
#include "settings/profile_store.h"
#include "ui/pages/pages.h"
#include "ui/widgets.h"

namespace ui::pages
{
namespace
{
constexpr float kProfileListHeight = 130.0f; // at UI scale 1.0
constexpr float kNameInputWidth = 220.0f;
constexpr float kPresetButtonWidth = 90.0f;

void request(app::AppState& app, app::ProfileOp op, std::string name = {})
{
    app.requests.profile = app::ProfileRequest{op, std::move(name)};
}

// "default" first, then the saved profiles. Click selects, double-click loads; the current one is marked.
void draw_profile_list(PageContext& ctx)
{
    app::AppState& app = ctx.app;
    MenuState& menu = ctx.menu;
    const std::vector<std::string>& names = app.profiles.names;
    if (std::find(names.begin(), names.end(), menu.selected_profile) == names.end())
    {
        menu.selected_profile = app.profiles.current;
    }
    const ImVec2 size(std::min(scaled(kNameInputWidth * 1.6f), ImGui::GetContentRegionAvail().x),
                      scaled(kProfileListHeight));
    if (!ImGui::BeginListBox("##profiles", size))
    {
        return;
    }
    for (const std::string& name : names)
    {
        const bool current = name == app.profiles.current;
        const std::string label = current ? name + "  (loaded)" : name;
        if (ImGui::Selectable(label.c_str(), name == menu.selected_profile, ImGuiSelectableFlags_AllowDoubleClick))
        {
            menu.selected_profile = name;
            menu.confirm_delete.clear();
            if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
            {
                request(app, app::ProfileOp::load, name);
            }
        }
    }
    ImGui::EndListBox();
}

void draw_profiles(PageContext& ctx)
{
    app::AppState& app = ctx.app;
    MenuState& menu = ctx.menu;
    const app::ProfileState& profiles = app.profiles;
    const bool current_read_only = settings::ProfileStore::is_read_only(profiles.current);

    widgets::card_begin(ctx.fonts, "Profiles");
    widgets::hint("Every setting and keybind, saved as a JSON file. The last loaded or saved profile loads again on "
                  "the next start. \"default\" is the built-in defaults and can't be overwritten: use Save as.");
    widgets::info_row("Current profile", profiles.current.c_str());
    if (app.unsaved_changes())
    {
        ImGui::SameLine();
        ImGui::TextColored(theme::kWarn, "%s", "unsaved changes");
    }

    draw_profile_list(ctx);

    // The selected profile: load or delete.
    if (ImGui::Button("Load"))
    {
        request(app, app::ProfileOp::load, menu.selected_profile);
    }
    ImGui::SameLine();
    const bool can_delete = !settings::ProfileStore::is_read_only(menu.selected_profile);
    ImGui::BeginDisabled(!can_delete);
    if (can_delete && menu.confirm_delete == menu.selected_profile)
    {
        ImGui::PushStyleColor(ImGuiCol_Text, theme::kDanger);
        if (ImGui::Button("Really delete?"))
        {
            request(app, app::ProfileOp::remove, menu.selected_profile);
            menu.confirm_delete.clear();
        }
        ImGui::PopStyleColor();
    }
    else if (ImGui::Button("Delete"))
    {
        menu.confirm_delete = menu.selected_profile;
    }
    ImGui::EndDisabled();

    // The current profile: save, or back to the defaults.
    ImGui::SameLine();
    ImGui::BeginDisabled(current_read_only);
    if (ImGui::Button("Save"))
    {
        request(app, app::ProfileOp::save);
    }
    ImGui::EndDisabled();
    ImGui::SameLine();
    if (ImGui::Button("Reset to defaults"))
    {
        request(app, app::ProfileOp::reset);
    }
    ImGui::SameLine();
    widgets::help_marker("Every setting and keybind back to the built-in defaults. Nothing is saved until you Save "
                         "(or Save as).");

    // A name: save as a new profile, or rename the current one.
    ImGui::SetNextItemWidth(std::min(scaled(kNameInputWidth), ImGui::GetContentRegionAvail().x));
    ImGui::InputTextWithHint("##profile_name", "new profile name", menu.profile_name.data(), menu.profile_name.size());
    const std::string name(menu.profile_name.data());
    const bool valid_name = settings::ProfileStore::clean_name(name).has_value();
    ImGui::SameLine();
    ImGui::BeginDisabled(!valid_name);
    if (ImGui::Button("Save as"))
    {
        request(app, app::ProfileOp::save_as, name);
        menu.profile_name.fill('\0');
    }
    ImGui::SameLine();
    ImGui::BeginDisabled(current_read_only);
    if (ImGui::Button("Rename current"))
    {
        request(app, app::ProfileOp::rename, name);
        menu.profile_name.fill('\0');
    }
    ImGui::EndDisabled();
    ImGui::EndDisabled();
    ImGui::SameLine();
    widgets::help_marker("Names: letters, digits, spaces, _ and -, up to 40 characters.");

    if (!profiles.message.empty())
    {
        widgets::notice(profiles.message.c_str(), profiles.message_failed);
    }
    if (!profiles.warnings.empty())
    {
        const std::string title = std::to_string(profiles.warnings.size()) + " problem(s) in the loaded profile";
        ImGui::PushStyleColor(ImGuiCol_Text, theme::kWarn);
        const bool open = ImGui::TreeNode(title.c_str());
        ImGui::PopStyleColor();
        if (open)
        {
            for (const std::string& warning : profiles.warnings)
            {
                ImGui::BulletText("%s", warning.c_str());
            }
            ImGui::TreePop();
        }
    }
    ImGui::TextColored(theme::kTextFaint, "Folder: %s", profiles.folder.c_str());
    widgets::card_end();
}

void draw_presets(PageContext& ctx)
{
    widgets::card_begin(ctx.fonts, "Presets");
    widgets::hint("A preset switches the features and sets their strength. Keybinds, colours, team mode, team checks, "
                  "max distances and where things sit stay as they are. Save to keep the result. Keys: Keybinds page, "
                  "Presets.");
    for (const settings::Preset preset : settings::kPresets)
    {
        const std::string label = std::string(settings::preset_name(preset)) + "##preset";
        const bool rage = preset == settings::Preset::rage;
        if (rage)
        {
            ImGui::PushStyleColor(ImGuiCol_Text, theme::kDanger);
        }
        if (ImGui::Button(label.c_str(), ImVec2(scaled(kPresetButtonWidth), 0.0f)))
        {
            ctx.app.requests.preset = preset;
        }
        if (rage)
        {
            ImGui::PopStyleColor();
        }
        ImGui::SameLine();
        ImGui::AlignTextToFramePadding();
        ImGui::PushTextWrapPos(0.0f);
        ImGui::TextColored(theme::kTextDim, "%s", settings::preset_description(preset).data());
        ImGui::PopTextWrapPos();
    }
    widgets::card_end();
}

void draw_overlay(PageContext& ctx)
{
    widgets::card_begin(ctx.fonts, "Overlay");
    ImGui::Checkbox("Watermark", &ctx.app.settings.overlay.watermark);
    ImGui::SameLine();
    widgets::help_marker("The logo and \"External Cheat by BigH\" in the top-left corner of the game, with the "
                         "features that are on listed under it.");
    ImGui::Checkbox("Frame outline", &ctx.app.settings.overlay.frame_outline);
    ImGui::SameLine();
    widgets::help_marker("A thin line along the overlay's edges. If the overlay covers the game exactly, the line sits "
                         "right on the edges of the game's picture, in windowed mode too.");
    widgets::card_end();
}
} // namespace

void draw_settings(PageContext& ctx)
{
    widgets::page_header(ctx.fonts, "Settings", "Profiles, presets and the overlay. Changes apply immediately.");
    draw_profiles(ctx);
    draw_presets(ctx);
    draw_overlay(ctx);
    widgets::planned_card(ctx.fonts, "Phases 9-10",
                          {"An exit button and the robustness pass (Phase 9)", "Menu size (Phase 10)"});
}
} // namespace ui::pages
