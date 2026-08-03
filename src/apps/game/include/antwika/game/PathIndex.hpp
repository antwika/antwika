#pragma once

#include <cstddef>
#include <set>

#include "antwika/game/Cell.hpp"
#include "antwika/game/Walking.hpp"

namespace antwika::game
{

    /**
     * @brief Which cells have a path on them.
     *
     * A lookup index beside the World, not a second copy of the truth: the
     * path entities own their Cell and Path components, and this answers
     * "is there one here" without a linear scan of every entity. A walker
     * asks that four times per tick, once per neighbour.
     *
     * A std::set rather than an unordered_set, because iteration order
     * reaches the drawing calls, and a hash seed must not be able to
     * decide what gets drawn first.
     *
     * Whatever creates a path entity is responsible for recording it here
     * too -- GridSink is the only thing that does either.
     */
    class PathIndex final
    {
    public:
        /**
         * @brief Record that a cell now has a path.
         *
         * Recording the same cell twice changes nothing, which is what
         * makes a second click on a laid tile a no-op rather than a
         * duplicate.
         *
         * @param cell The cell to record.
         * @return True if this added the cell, false if it was already
         * there.
         */
        bool insert(Cell cell);

        /**
         * @brief Record that a cell's path is gone.
         *
         * The raze tool is what takes one away, and whatever destroys
         * the path entity is responsible for erasing it here too --
         * exactly insert()'s contract read the other way round.
         *
         * @param cell The cell to clear.
         * @return True if this removed a path, false if none was there.
         */
        bool erase(Cell cell);

        /**
         * @brief Check whether a cell has a path.
         * @param cell The cell to ask about.
         * @return True when a path has been recorded there.
         */
        [[nodiscard]] bool has(Cell cell) const;

        /**
         * @brief Get which of a cell's four neighbours have paths.
         * @param cell The cell to look around.
         * @return The neighbouring paths, for nextFacing().
         */
        [[nodiscard]] Neighbours neighboursOf(Cell cell) const;

        /**
         * @brief Get how many cells have paths.
         * @return The count.
         */
        [[nodiscard]] std::size_t size() const noexcept;

        /**
         * @brief Get every path cell, in ascending order.
         * @return The cells, ordered as Cell orders them.
         */
        [[nodiscard]] const std::set<Cell> &cells() const noexcept;

    private:
        std::set<Cell> paths;
    };

} // namespace antwika::game
