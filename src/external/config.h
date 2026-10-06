#pragma once

// Non-offset constants and tunables. Offsets live in game/offsets.h and game/schema.h, never here.
//
// PURE: no <Windows.h>.

#include <cstdint>
#include <string_view>

namespace config
{
static_assert(sizeof(void*) == 8, "x64 only");

// User-mode address range of an x64 Windows process. The first 64 KiB are never mapped.
inline constexpr std::uintptr_t kMinValidPointer = 0x10000;
inline constexpr std::uintptr_t kMaxValidPointer = 0x7FFFFFFFFFFF;

// The game and the modules we read from.
inline constexpr std::wstring_view kGameExe = L"cs2.exe";
inline constexpr std::wstring_view kClientModule = L"client.dll";
inline constexpr std::wstring_view kEngineModule = L"engine2.dll";
} // namespace config
