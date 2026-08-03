#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include "antwika/game/BuildingKind.hpp"
#include "antwika/game/Cell.hpp"
#include "antwika/game/GridExtent.hpp"

namespace antwika::game
{

    /**
     * @brief How many cells a building covers, across and down.
     *
     * **Square only, and that restriction pays for itself.**
     * A cell's bounding box is twice the camera's half-width by twice
     * its half-height, and halfHeight is halfWidth / 2, so every
     * footprint's box comes out 2:1 -- the same shape as one atlas tile.
     * A square block therefore *is* a diamond, and its art is one
     * ordinary tile scaled up with no geometric error and no atlas work.
     * A 2x3 block is a hexagon rather than a diamond, so a scaled square
     * tile is visibly wrong on one and it would need a source rect of
     * its own, which is a half-tile-quantised band in the atlas and a
     * much heavier contract with whoever draws it.
     *
     * Signed to match Cell and GridExtent, so no arithmetic here ever
     * mixes signedness; gfx::Size is unsigned pixels and is the wrong
     * type for a count of cells.
     */
    struct Footprint
    {
        std::int32_t width = 1;
        std::int32_t height = 1;

        /**
         * @brief Compare two footprints.
         * @param other The footprint to compare against.
         * @return True when both extents match.
         */
        [[nodiscard]] constexpr bool operator==(
            const Footprint &other) const = default;
    };

    /**
     * @brief How big each kind of building is.
     *
     * A table keyed by kind rather than a field on the component, for
     * three reasons.
     * A field could disagree with the kind that placed it.
     * BuildingKind's values are contiguous precisely so a kind can index
     * a table.
     * And the placement ghost has to know the size *before* any entity
     * exists, which a component cannot tell it.
     */
    inline constexpr std::array<Footprint, kBuildingKindCount> kFootprints{{
        {.width = 1, .height = 1}, // House
        {.width = 2, .height = 2}, // Farm
        {.width = 2, .height = 2}, // ClayPit
        {.width = 2, .height = 2}, // Workshop
        {.width = 3, .height = 3}, // Storage
        {.width = 2, .height = 2}, // Market
        {.width = 1, .height = 1}, // Well
        {.width = 1, .height = 1}, // Doctor
        {.width = 1, .height = 1}, // FireStation
        {.width = 1, .height = 1}, // EngineerPost
    }};

    /**
     * @brief Get how big a building of this kind is.
     * @param kind The kind to size.
     * @return Its footprint.
     */
    [[nodiscard]] constexpr Footprint footprintOf(BuildingKind kind) noexcept
    {
        return kFootprints[buildingKindIndex(kind) % kBuildingKindCount];
    }

    /**
     * @brief Check whether a footprint placed here covers a cell.
     *
     * The origin is the minimum-x, minimum-y cell, which under this
     * projection is also the block's topmost corner -- so one definition
     * serves the range test, the map key and the blit anchor rather than
     * needing a conversion between two of them.
     *
     * @param origin Where the building was placed.
     * @param footprint How big it is.
     * @param cell The cell to ask about.
     * @return True when the cell is one the building stands on.
     */
    [[nodiscard]] constexpr bool covers(
        Cell origin, Footprint footprint, Cell cell) noexcept
    {
        return cell.x >= origin.x && cell.x < origin.x + footprint.width
            && cell.y >= origin.y && cell.y < origin.y + footprint.height;
    }

    /**
     * @brief Check whether a footprint placed here is wholly in bounds.
     * @param origin Where the building would go.
     * @param footprint How big it is.
     * @param extent The bounds it must fit inside.
     * @return True when every cell it would cover is in the extent.
     */
    [[nodiscard]] constexpr bool fitsIn(
        Cell origin, Footprint footprint, GridExtent extent) noexcept
    {
        // The two far corners are enough.
        // The block is a rectangle and the extent is one too.
        return extent.contains(origin)
            && extent.contains(Cell{
                   .x = origin.x + footprint.width - 1,
                   .y = origin.y + footprint.height - 1});
    }

    static_assert(footprintOf(BuildingKind::House) == Footprint{1, 1});
    static_assert(covers(Cell{}, Footprint{2, 2}, Cell{.x = 1, .y = 1}));
    static_assert(!covers(Cell{}, Footprint{2, 2}, Cell{.x = 2, .y = 0}));

    /**
     * @brief Check whether a cell is next to a block without being in it.
     *
     * **What "has arrived" means for anybody walking to a building.**
     * Nothing stands on a block but the building itself, so a walker
     * heading for one stops on the road beside it -- and every system
     * that reacts to an arrival has to ask the same question the walker
     * stopped on, or it waits for a cell the walker will never reach.
     *
     * Any cell of the block will do, which is why spawnCellFor() walks
     * the whole perimeter and stepTowards() makes every cell of the goal
     * passable.
     *
     * @param at Where the walker is.
     * @param origin Where the building was placed.
     * @param footprint How big it is.
     * @return True when a single step from `at` lands on the block.
     */
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

    // A one-cell block is not beside itself.
    // Nothing ever stands on a block, so this is the shape of the rule.
    static_assert(!beside(Cell{}, Cell{}, Footprint{1, 1}));

    // Square only, and the whole art argument above rests on it.
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

} // namespace antwika::game
