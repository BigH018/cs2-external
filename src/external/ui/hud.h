#pragma once

// What the overlay draws on the game every frame, menu open or not (ImGui's background draw list): the watermark and
// the optional frame outline. Phase 4 adds the ESP next to it through render/painter.

#include <imgui.h>

#include "app/state.h"
#include "ui/theme.h"

namespace ui
{
// `logo` may be null (then the watermark has no logo).
void draw_hud(const Fonts& fonts, ImTextureData* logo, const app::AppState& app);
} // namespace ui
