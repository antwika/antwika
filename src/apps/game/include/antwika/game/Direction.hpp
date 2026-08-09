#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include <antwika/enums/Enumeration.hpp>

#include "antwika/game/Cell.hpp"

namespace antwika::game
{

    enum class Direction : std::uint8_t
    {
        North = 0,
        East,
        South,
        West,
    };

    [[nodiscard]] constexpr Direction enumBound(Direction) noexcept
    {
        return Direction::West;
    }

    inline constexpr std::size_t kDirectionCount =
        antwika::enums::kCount<Direction>;

    [[nodiscard]] constexpr std::size_t directionIndex(
        const Direction direction) noexcept
    {
        return antwika::enums::index(direction);
    }

    [[nodiscard]] constexpr Direction turnRight(Direction direction) noexcept
    {
        return static_cast<Direction>(
            (directionIndex(direction) + 1) % kDirectionCount);
    }

    [[nodiscard]] constexpr Direction turnLeft(Direction direction) noexcept
    {
        return static_cast<Direction>(
            (directionIndex(direction) + kDirectionCount - 1)
            % kDirectionCount);
    }

    [[nodiscard]] constexpr Direction opposite(Direction direction) noexcept
    {
        return static_cast<Direction>(
            (directionIndex(direction) + 2) % kDirectionCount);
    }

    [[nodiscard]] constexpr Cell step(Cell from, Direction direction) noexcept
    {
        constexpr std::array<Cell, kDirectionCount> deltas{{
            {.x = 0, .y = -1},
            {.x = 1, .y = 0},
            {.x = 0, .y = 1},
            {.x = -1, .y = 0},
        }};

        const auto delta =
            deltas[directionIndex(direction) % kDirectionCount];

        return Cell{.x = from.x + delta.x, .y = from.y + delta.y};
    }

}
