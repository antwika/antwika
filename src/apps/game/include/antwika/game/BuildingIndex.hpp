#pragma once

#include <cstddef>
#include <set>

#include "antwika/game/Cell.hpp"
#include "antwika/game/Footprint.hpp"

namespace antwika::game
{

    /**
     * @brief Which cells have a building on them.
     *
     * PathIndex's counterpart, and it exists for the same reason: the
     * World stages what a tick writes and only hands it over at the next
     * commit(), so two clicks on one cell inside a tick would both find
     * the ground empty and put two buildings on it.
     * This answers "is there one here" as of right now rather than as of
     * the last commit.
     *
     * A std::set rather than an unordered one, so nothing about a hash
     * seed can reach what gets drawn or demolished first.
     *
     * **Two things keep it in step with the world, and only two.**
     * GridSink records a cell as it builds on one, and BuildingSystem
     * clears one as it demolishes. A third writer would be a third place
     * this could drift from the World, which is exactly the failure it
     * exists to prevent.
     */
    class BuildingIndex final
    {
    public:
        /**
         * @brief Record a building standing on a block of cells.
         *
         * Every covered cell is recorded, not only the origin, so a
         * road laid against the far corner of a three-by-three finds
         * something there.
         *
         * @param origin The minimum-x, minimum-y cell of the block.
         * @param footprint How many cells across and down it covers.
         * @return True if this added it, false if any covered cell was
         * already taken -- in which case nothing is recorded at all.
         */
        bool insert(Cell origin, Footprint footprint);

        /**
         * @brief Forget the building standing on a block of cells.
         * @param origin The minimum-x, minimum-y cell of the block.
         * @param footprint How many cells across and down it covers.
         * @return True if this removed anything.
         */
        bool erase(Cell origin, Footprint footprint);

        /**
         * @brief Check whether a cell has a building standing on it.
         * @param cell The cell to ask about.
         * @return True when some building covers it.
         */
        [[nodiscard]] bool has(Cell cell) const;

        /**
         * @brief Check whether a whole block is clear.
         * @param origin Where the block would start.
         * @param footprint How many cells across and down it covers.
         * @return True when no covered cell already has a building.
         */
        [[nodiscard]] bool free(Cell origin, Footprint footprint) const;

        /**
         * @brief Get how many cells have buildings.
         * @return The count.
         */
        [[nodiscard]] std::size_t size() const noexcept;

        /**
         * @brief Get every built cell, in ascending order.
         * @return The cells, ordered as Cell orders them.
         */
        [[nodiscard]] const std::set<Cell> &cells() const noexcept;

    private:
        std::set<Cell> occupied;
    };

} // namespace antwika::game
