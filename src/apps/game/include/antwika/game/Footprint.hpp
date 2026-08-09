#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include <antwika/enums/Enumeration.hpp>

#include "antwika/game/BuildingKind.hpp"
#include "antwika/game/Cell.hpp"
#include "antwika/game/GridExtent.hpp"

namespace antwika::game
{

    struct Footprint final
    {
        std::int32_t width = 1;
        std::int32_t height = 1;

        [[nodiscard]] constexpr bool operator==(
            const Footprint &other) const = default;
    };

    inline constexpr std::array<Footprint, kBuildingKindCount> kFootprints{{
        {.width = 1, .height = 1},
        {.width = 2, .height = 2},
        {.width = 2, .height = 2},
        {.width = 2, .height = 2},
        {.width = 3, .height = 3},
        {.width = 2, .height = 2},
        {.width = 1, .height = 1},
        {.width = 1, .height = 1},
        {.width = 1, .height = 1},
        {.width = 1, .height = 1},
    }};

    [[nodiscard]] constexpr Footprint footprintOf(BuildingKind kind) noexcept
    {
        return antwika::enums::pick(kFootprints, kind);
    }

    [[nodiscard]] constexpr bool covers(
        Cell origin, Footprint footprint, Cell cell) noexcept
    {
        return cell.x >= origin.x && cell.x < origin.x + footprint.width
            && cell.y >= origin.y && cell.y < origin.y + footprint.height;
    }

    [[nodiscard]] constexpr bool fitsIn(
        Cell origin, Footprint footprint, GridExtent extent) noexcept
    {
        return extent.contains(origin)
            && extent.contains(Cell{
                   .x = origin.x + footprint.width - 1,
                   .y = origin.y + footprint.height - 1});
    }

    static_assert(footprintOf(BuildingKind::House) == Footprint{1, 1});
    static_assert(covers(Cell{}, Footprint{2, 2}, Cell{.x = 1, .y = 1}));
    static_assert(!covers(Cell{}, Footprint{2, 2}, Cell{.x = 2, .y = 0}));

    [[nodiscard]] constexpr bool beside(
        Cell at, Cell origin, Footprint footprint) noexcept
    {
        return covers(origin, footprint, Cell{.x = at.x, .y = at.y - 1})
            || covers(origin, footprint, Cell{.x = at.x + 1, .y = at.y})
            || covers(origin, footprint, Cell{.x = at.x, .y = at.y + 1})
            || covers(origin, footprint, Cell{.x = at.x - 1, .y = at.y});
    }

    static_assert(beside(Cell{.x = 0, .y = 1}, Cell{}, Footprint{1, 1}));
    static_assert(beside(Cell{.x = 2, .y = 1}, Cell{}, Footprint{2, 2}));
    static_assert(!beside(Cell{.x = 3, .y = 1}, Cell{}, Footprint{2, 2}));

    static_assert(!beside(Cell{}, Cell{}, Footprint{1, 1}));

    static_assert(
        []
        {
            for (const auto footprint : kFootprints)
            {
                if (footprint.width != footprint.height)
                {
                    return false;
                }
            }

            return true;
        }(),
        "a footprint must be square, or its art is not a diamond");

}
