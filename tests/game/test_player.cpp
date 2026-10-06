#include <cstdint>

#include <doctest.h>

#include "game/offsets.h"
#include "game/player.h"
#include "helpers/fake_memory.h"

namespace
{
constexpr std::uintptr_t kClientBase = 0x7FFD62930000;
constexpr std::uintptr_t kLocalPawnSlot = kClientBase + game::offsets::client::dwLocalPlayerPawn;
} // namespace

TEST_CASE("read_local_pawn reads client.dll + dwLocalPlayerPawn")
{
    test::FakeMemory memory;
    memory.map_value<std::uintptr_t>(kLocalPawnSlot, 0x4DF564BF800);
    CHECK(game::read_local_pawn(memory, kClientBase) == 0x4DF564BF800);
}

TEST_CASE("read_local_pawn: 0 in the main menu, nullopt when the read fails")
{
    test::FakeMemory memory;
    memory.map_value<std::uintptr_t>(kLocalPawnSlot, 0);
    CHECK(game::read_local_pawn(memory, kClientBase) == std::uintptr_t{0});

    const test::FakeMemory empty;
    CHECK_FALSE(game::read_local_pawn(empty, kClientBase).has_value());
}
