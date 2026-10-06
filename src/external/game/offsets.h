#pragma once

// Module-base RVAs (stable globals). Every value records where it came from.
//
// Source for this file unless noted otherwise:
//   a2x/cs2-dumper output for CS2 build 14189, dumped 2026-10-06T16:04:45Z (docs/dumps/info.json),
//   copied from docs/dumps/offsets.json.
//
// Names match the dumper output exactly (not kPascalCase) so they can be grepped across dumps.
// Phase 2 copies in the rest of the dw* globals; Phase 0 only needs the local pawn.
//
// PURE: no <Windows.h>.

#include <cstdint>

namespace game::offsets::client
{
// client.dll + dwLocalPlayerPawn -> C_CSPlayerPawn* (0 when not in a match). Dump value 39200776.
// Verified in-game 2026-10-06 (build 14189): 0x4DF564BF800 in an offline deathmatch.
inline constexpr std::uintptr_t dwLocalPlayerPawn = 0x2562808;
} // namespace game::offsets::client
