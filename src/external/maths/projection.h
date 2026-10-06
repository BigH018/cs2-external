#pragma once

// The game's view matrix (world -> clip space). Phase 3 reads it and checks it; Phase 4 adds world_to_screen.
//
// Source 2 keeps it as 16 floats, row-major: clip.x = m[0]x + m[1]y + m[2]z + m[3], ..., clip.w = m[12]x + m[13]y +
// m[14]z + m[15]. Read 2026-10-06 in a bot match (build 14189): the w row is the camera's forward axis plus a
// translation, as row-major predicts.
//
// PURE: no <Windows.h>.

#include <array>
#include <cmath>

namespace maths
{
struct ViewMatrix
{
    std::array<float, 16> m{};

    [[nodiscard]] constexpr float at(int row, int column) const noexcept { return m[row * 4 + column]; }

    // A matrix worth projecting with: every value finite, and not all zero (the game leaves it zeroed until the first
    // frame of a match is rendered).
    [[nodiscard]] bool is_sane() const noexcept
    {
        bool any_nonzero = false;
        for (const float value : m)
        {
            if (!std::isfinite(value))
            {
                return false;
            }
            any_nonzero = any_nonzero || value != 0.0f;
        }
        return any_nonzero;
    }
};

static_assert(sizeof(ViewMatrix) == 64, "ViewMatrix must match the game's 4x4 float layout");
} // namespace maths
