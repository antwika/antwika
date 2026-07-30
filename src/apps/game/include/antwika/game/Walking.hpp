#pragma once

#include <array>
#include <optional>

#include "antwika/game/Direction.hpp"

namespace antwika::game
{

    /**
     * @brief Which of a cell's four neighbours have a path on them.
     *
     * Four named flags rather than a bitmask, for the reason
     * input::KeyModifiers gives: it is what makes the value legible at the
     * call site, and nothing needs to combine them arithmetically.
     */
    struct Neighbours
    {
        bool north = false;
        bool east = false;
        bool south = false;
        bool west = false;

        /**
         * @brief Compare two sets of neighbours.
         * @param other The set to compare against.
         * @return True when the same neighbours are present in both.
         */
        [[nodiscard]] bool operator==(const Neighbours &other) const = default;
    };

    /**
     * @brief Check whether one particular neighbour has a path.
     * @param neighbours The neighbours to look in.
     * @param direction The one to ask about.
     * @return True when that neighbour has a path.
     */
    [[nodiscard]] constexpr bool has(
        Neighbours neighbours, Direction direction) noexcept
    {
        const std::array<bool, kDirectionCount> present{
            neighbours.north,
            neighbours.east,
            neighbours.south,
            neighbours.west};

        return present[directionIndex(direction) % kDirectionCount];
    }

    /**
     * @brief Get the direction a walker facing one way leaves a cell in.
     *
     * Tries right, then straight on, then left, then back, and takes the
     * first that has a path.
     *
     * That single order is both rules the walkers are meant to follow, and
     * it is worth saying why they are one rule rather than two. "Prefers to
     * turn right at an intersection" is the order's first entry. "Reverses
     * at a dead end" is `back` being its last: a walker turns round only
     * when nothing else is available, which is what a dead end *is*. No
     * branch tests for one.
     *
     * @param facing The direction the walker arrived facing.
     * @param neighbours Which of the current cell's neighbours have paths.
     * @return The direction to leave in, or nullopt when the cell has no
     * path neighbour at all -- a walker dropped on a one-tile path has
     * nowhere to go, including backwards, and a rule forced to return
     * something would have to invent a move.
     */
    [[nodiscard]] constexpr std::optional<Direction> nextFacing(
        Direction facing, Neighbours neighbours) noexcept
    {
        const std::array<Direction, kDirectionCount> preference{
            turnRight(facing),
            facing,
            turnLeft(facing),
            opposite(facing)};

        for (const auto candidate : preference)
        {
            if (has(neighbours, candidate))
            {
                return candidate;
            }
        }

        return std::nullopt;
    }

} // namespace antwika::game
