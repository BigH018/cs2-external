#pragma once

// Keybind controls shared by the Keybinds page and the feature pages. They edit app.settings.keybinds and start or
// cancel app.capture; app/frame polls the keys and stores a captured key.

#include "app/state.h"
#include "input/actions.h"

namespace ui::keybind
{
// The bound key. Click: "Press a key..." (the next key or mouse button becomes the bind, Esc clears it, clicking again
// cancels). Red with a tooltip when the same key is bound to other actions.
void key_button(app::AppState& app, input::ActionId id);

// Hold / Toggle for actions that allow both (nothing for press-only actions).
void mode_selector(app::AppState& app, input::ActionId id);

// "Label    [key] [Hold v]" on one line, for the feature pages.
void bind_row(app::AppState& app, input::ActionId id, const char* label);
} // namespace ui::keybind
