#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>

#include <antwika/rng/SplitMix64Rng.hpp>
#include <antwika/time/Tick.hpp>

#include "antwika/game/Cell.hpp"
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
     * **Anything but back the way it came, chosen at random among what
     * is left, and back the way it came only when nothing is.**
     *
     * The two halves of that are the two rules the walkers follow, and
     * they are still one rule rather than two: reversing is what is left
     * when the set of everything else is empty, which is what a dead end
     * *is*, so no branch tests for one.
     *
     * It used to prefer a right turn, then straight on, then left.
     * That is deterministic, cheap and reads as a bug: every walker
     * leaving a junction the same way makes a district's traffic run in
     * visible circles, and a whole city's walkers file round the same
     * block.
     * Choosing among the arms instead spreads them over the network
     * without making any of it less reproducible -- the bits come from
     * the caller, and WalkerSystem draws them from the tick, the cell
     * and the way the walker is facing, every one of which a replay and
     * a reloaded save reach again.
     *
     * The candidates are collected in Direction's own order, so which
     * arm a given roll picks is a function of the enumeration rather
     * than of the order the neighbours were asked about.
     *
     * @param facing The direction the walker arrived facing.
     * @param neighbours Which of the current cell's neighbours have paths.
     * @param roll Random bits to choose among the arms with; see
     * wanderRoll() for where a caller inside the tick path gets them.
     * @return The direction to leave in, or nullopt when the cell has no
     * path neighbour at all -- a walker dropped on a one-tile path has
     * nowhere to go, including backwards, and a rule forced to return
     * something would have to invent a move.
     */
    [[nodiscard]] constexpr std::optional<Direction> nextFacing(
        Direction facing,
        Neighbours neighbours,
        std::uint64_t roll) noexcept
    {
        const auto back = opposite(facing);

        std::array<Direction, kDirectionCount> onwards{};
        std::size_t count = 0;

        for (std::size_t index = 0; index < kDirectionCount; ++index)
        {
            const auto candidate = static_cast<Direction>(index);

            if (candidate != back && has(neighbours, candidate))
            {
                onwards[count] = candidate;
                ++count;
            }
        }

        if (count > 0)
        {
            // A plain modulo, since count is never above three.
            // The bias that leaves is one part in six thousand million.
            // Splitmix64's low bits are as good as its high ones.
            return onwards[static_cast<std::size_t>(roll % count)];
        }

        // A dead end, which is the one place a walker turns round.
        // Or a cell with nothing off it at all, which is neither.
        if (has(neighbours, back))
        {
            return back;
        }

        return std::nullopt;
    }

    /**
     * @brief Get the bits a walker chooses its next arm with.
     *
     * **Stateless on purpose, and that is the whole design.** A
     * generator advanced once per decision would be state living in a
     * system rather than in the World, so a save would not cover it and
     * a city reloaded would roam differently from the one that was
     * saved -- which is the very thing stepTowards() refuses to keep a
     * route in a component for.
     *
     * Seeded instead from three things a replay and a restore both
     * reach again: which tick it is, which cell the walker is standing
     * on, and which way it came in.
     * Two walkers meeting on one cell facing the same way on one tick
     * therefore turn the same way, which is not a collision anybody has
     * to resolve -- walkers do not collide, and two of them leaving a
     * junction together is a thing that happens in a city.
     *
     * The tick is in the seed rather than only the cell, since a walker
     * that comes round to the same junction facing the same way ought
     * to be able to make a different choice of it.
     *
     * @param tick Which tick the decision is being made on.
     * @param at The cell the walker is leaving.
     * @param facing The direction it arrived facing.
     * @return Bits to hand nextFacing().
     */
    [[nodiscard]] inline std::uint64_t wanderRoll(
        antwika::time::Tick tick, Cell at, Direction facing) noexcept
    {
        // Mixed into one word before the generator sees it.
        // Odd multipliers, so no two of the three can cancel out.
        // A plain sum would make (x, y) and (y, x) the one seed.
        const auto seed = tick * 0x9E3779B97F4A7C15ULL
            + static_cast<std::uint64_t>(
                  static_cast<std::uint32_t>(at.x))
                  * 0xBF58476D1CE4E5B9ULL
            + static_cast<std::uint64_t>(
                  static_cast<std::uint32_t>(at.y))
                  * 0x94D049BB133111EBULL
            + static_cast<std::uint64_t>(directionIndex(facing));

        return antwika::rng::SplitMix64Rng(seed).next();
    }

} // namespace antwika::game
