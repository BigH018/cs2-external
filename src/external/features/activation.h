#pragma once

// Hold / toggle activation from a key's up/down state, polled once per frame. Phase 7's keybind engine generalises
// this to every action.
//
// PURE: no <Windows.h>, no ImGui.

#include "settings/settings.h"

namespace features
{
class KeyActivation
{
public:
    // `key_down`: the key's state this frame. Returns whether the feature is on.
    bool update(bool key_down, settings::BindMode mode) noexcept
    {
        const bool pressed = key_down && !was_down_;
        was_down_ = key_down;
        if (mode == settings::BindMode::hold)
        {
            toggled_ = false;
            return key_down;
        }
        if (pressed)
        {
            toggled_ = !toggled_;
        }
        return toggled_;
    }

    void reset() noexcept
    {
        was_down_ = false;
        toggled_ = false;
    }

private:
    bool was_down_ = false;
    bool toggled_ = false;
};
} // namespace features
