#include "game/player.h"

#include "game/offsets.h"

namespace game
{
std::optional<std::uintptr_t> read_local_pawn(const core::Memory& memory, std::uintptr_t client_base) noexcept
{
    return memory.read<std::uintptr_t>(client_base + offsets::client::dwLocalPlayerPawn);
}
} // namespace game
