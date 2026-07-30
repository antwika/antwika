#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include "antwika/game/Cell.hpp"

namespace antwika::game
{

    /**
     * @brief One of the four ways along the grid.
     *
     * Defined in **grid** space, not screen space: North is -y, East is
     * +x, South is +y, West is -x.
     *
     * That distinction usually matters and here it does not, which is
     * worth saying because "turn right" in an isometric view invites the
     * question. The projection is one fixed shear and scale, the same for
     * every cell at every zoom level, so a clockwise step in grid space is
     * a clockwise step on screen too. The two readings coincide, and
     * turnRight() means both.
     *
     * Values are contiguous from zero, so a direction can index a table.
     */
    enum class Direction : std::uint8_t
    {
        North = 0,
        East,
        South,
        West,
    };

    /**
     * @brief How many directions there are.
     *
     * Derived from the last enumerator rather than written out, so it
     * cannot drift from the enumeration it counts.
     */
    inline constexpr std::size_t kDirectionCount =
        static_cast<std::size_t>(Direction::West) + 1;

    /**
     * @brief Get a direction's index, for addressing a per-direction
     * table.
     * @param direction The direction to index.
     * @return The index, always below kDirectionCount for a named
     * direction.
     */
    [[nodiscard]] constexpr std::size_t directionIndex(
        Direction direction) noexcept
    {
        return static_cast<std::size_t>(direction);
    }

    /**
     * @brief Get the direction a quarter turn clockwise.
     *
     * Arithmetic modulo kDirectionCount rather than a switch, so every
     * value has an answer and there is no unreachable default case for a
     * coverage gate to ask about.
     *
     * @param direction The direction to turn from.
     * @return The direction to its right.
     */
    [[nodiscard]] constexpr Direction turnRight(Direction direction) noexcept
    {
        return static_cast<Direction>(
            (directionIndex(direction) + 1) % kDirectionCount);
    }

    /**
     * @brief Get the direction a quarter turn anticlockwise.
     * @param direction The direction to turn from.
     * @return The direction to its left.
     */
    [[nodiscard]] constexpr Direction turnLeft(Direction direction) noexcept
    {
        return static_cast<Direction>(
            (directionIndex(direction) + kDirectionCount - 1)
            % kDirectionCount);
    }

    /**
     * @brief Get the direction straight back.
     * @param direction The direction to reverse.
     * @return The direction facing the other way.
     */
    [[nodiscard]] constexpr Direction opposite(Direction direction) noexcept
    {
        return static_cast<Direction>(
            (directionIndex(direction) + 2) % kDirectionCount);
    }

    /**
     * @brief Get the cell one step along a direction.
     * @param from The cell to leave.
     * @param direction The way to go.
     * @return The neighbouring cell in that direction.
     */
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

} // namespace antwika::game
