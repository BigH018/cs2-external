#pragma once

// Keys a feature can be bound to, by virtual-key code. Phase 5 picks from this list in a combo; Phase 7's keybind
// engine adds every key, key capture and the PRESS mode.
//
// PURE: no <Windows.h> (the codes are the documented VK_* values).

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace input
{
struct KeyInfo
{
    std::uint32_t vk;
    std::string_view name;
};

inline constexpr std::array kBindableKeys{
    KeyInfo{0x01, "Mouse 1"}, KeyInfo{0x02, "Mouse 2"}, KeyInfo{0x04, "Mouse 3"},   KeyInfo{0x05, "Mouse 4"},
    KeyInfo{0x06, "Mouse 5"}, KeyInfo{0x10, "Shift"},   KeyInfo{0x11, "Ctrl"},      KeyInfo{0x12, "Alt"},
    KeyInfo{0x14, "Caps Lock"}, KeyInfo{0x43, "C"},     KeyInfo{0x45, "E"},         KeyInfo{0x46, "F"},
    KeyInfo{0x51, "Q"},       KeyInfo{0x56, "V"},       KeyInfo{0x58, "X"},         KeyInfo{0x5A, "Z"},
};

// The key's name, or "?" if it isn't in the list.
[[nodiscard]] constexpr std::string_view key_name(std::uint32_t vk) noexcept
{
    for (const KeyInfo& key : kBindableKeys)
    {
        if (key.vk == vk)
        {
            return key.name;
        }
    }
    return "?";
}

// The key's position in kBindableKeys, or -1.
[[nodiscard]] constexpr int key_index(std::uint32_t vk) noexcept
{
    for (std::size_t i = 0; i < kBindableKeys.size(); ++i)
    {
        if (kBindableKeys[i].vk == vk)
        {
            return static_cast<int>(i);
        }
    }
    return -1;
}
} // namespace input
