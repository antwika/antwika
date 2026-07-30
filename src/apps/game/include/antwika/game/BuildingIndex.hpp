#pragma once

#include <cstddef>
#include <set>

#include "antwika/game/Cell.hpp"

namespace antwika::game
{

    /**
     * @brief Which cells have a building on them.
     *
     * PathIndex's counterpart, and it exists for the same reason: the
     * World stages what a tick writes and only shows it at the next
     * commit(), so two clicks on one cell inside a tick would both find
     * the ground empty and put two buildings on it.  This answers "is
     * there one here" as of right now rather than as of the last commit.
     *
     * A std::set rather than an unordered_set, so that nothing about a
     * hash seed can reach what gets drawn or destroyed first.
     *
     * Two things keep it in step with the world, and only two: GridSink
     * records a building as it puts one up, and BuildingSystem erases one
     * as it takes it down.
     */
    class BuildingIndex final
    {
    public:
        /**
         * @brief Record that a cell now has a building.
         * @param cell The cell to record.
         * @return True if this added the cell, false if it was already
         * there.
         */
        bool insert(Cell cell);

        /**
         * @brief Forget that a cell has a building.
         * @param cell The cell to clear.
         * @return True if this removed a record, false if there was none.
         */
        bool erase(Cell cell);

        /**
         * @brief Check whether a cell has a building.
         * @param cell The cell to ask about.
         * @return True when a building has been recorded there.
         */
        [[nodiscard]] bool has(Cell cell) const;

        /**
         * @brief Get how many cells have buildings.
         * @return The count.
         */
        [[nodiscard]] std::size_t size() const noexcept;

        /**
         * @brief Get every building cell, in ascending order.
         * @return The cells, ordered as Cell orders them.
         */
        [[nodiscard]] const std::set<Cell> &cells() const noexcept;

    private:
        std::set<Cell> occupied;
    };

} // namespace antwika::game
