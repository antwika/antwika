#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>

#include <antwika/enums/Enumeration.hpp>
#include <antwika/rng/SplitMix64Rng.hpp>
#include <antwika/time/Tick.hpp>

#include "antwika/game/Cell.hpp"
#include "antwika/game/Direction.hpp"

namespace antwika::game
{

    struct Neighbours final
    {
        bool north = false;
        bool east = false;
        bool south = false;
        bool west = false;

        [[nodiscard]] bool operator==(const Neighbours &other) const = default;
    };

    [[nodiscard]] constexpr bool has(
        Neighbours neighbours, Direction direction) noexcept
    {
        const std::array<bool, kDirectionCount> present{
            neighbours.north,
            neighbours.east,
            neighbours.south,
            neighbours.west};

        return antwika::enums::pick(present, direction);
    }

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
            return onwards[static_cast<std::size_t>(roll % count)];
        }

        if (has(neighbours, back))
        {
            return back;
        }

        return std::nullopt;
    }

    [[nodiscard]] inline std::uint64_t wanderRoll(
        antwika::time::Tick tick, Cell at, Direction facing) noexcept
    {
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

}
