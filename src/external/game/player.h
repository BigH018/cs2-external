#pragma once

// Player reads. Phase 0: the local pawn pointer only. Phase 3 adds snapshots and validity checks.

#include <cstdint>
#include <optional>

#include "core/memory.h"

namespace game
{
// Reads the local C_CSPlayerPawn pointer at client.dll + dwLocalPlayerPawn.
// nullopt = the read itself failed; 0 = no local pawn (main menu, loading screen).
[[nodiscard]] std::optional<std::uintptr_t> read_local_pawn(const core::Memory& memory,
                                                            std::uintptr_t client_base) noexcept;
} // namespace game
