#pragma once

#include <cstdint>

#include "antwika/tower_defence/Level.hpp"

namespace antwika::tower_defence
{

    /**
     * @brief Everything one level's generation is decided by.
     *
     * A seed rather than a random device, because a replay has to land
     * on the same level as the run that recorded it, and the level is
     * not persisted -- it is regenerated from this config alone.
     */
    struct LevelConfig
    {
        /** @brief Columns in the grid; at least 3. */
        std::uint32_t width = 12;

        /** @brief Rows in the grid; at least 1. */
        std::uint32_t height = 8;

        /** @brief Chooses the level; same seed, same level. */
        std::uint64_t seed = 0;

        /**
         * @brief Every n-th column is a wall with one gap in it.
         *
         * At least 2, so column 0 is never walled.
         * Each wall keeps exactly one open row, which is what forces the
         * path to weave rather than run straight across, while leaving
         * the grid connected so a solution always exists.
         */
        std::uint32_t wallSpacing = 3;

        /** @brief Solver step budget per attempt. */
        std::uint64_t maxSolverSteps = 20000;

        /** @brief How many re-seeded attempts before giving up. */
        std::uint32_t maxAttempts = 64;
    };

    /**
     * @brief Generate a level whose path is a single simple walk.
     *
     * Wave Function Collapse gives a constraint solver, not a path
     * guarantee, so linearity is arranged in three layers rather than
     * hoped for:
     *
     * 1. The tile alphabet cannot express a branch. Every tile is open
     *    on at most two sides (LevelTile.hpp), so no cell can have three
     *    neighbours and an intersection is not a symbol that exists.
     * 2. Exactly one Start and one End are allowed anywhere in the wave,
     *    and both are pinned to a chosen border cell. With every other
     *    cell having even degree, the graph the solution describes is a
     *    disjoint union of simple cycles plus exactly one simple path,
     *    whose two ends are the only degree-one cells -- Start and End.
     *    Walking out of Start therefore always arrives at End.
     * 3. A solution may still contain stray cycles somewhere else in the
     *    grid, since a cycle breaks no adjacency rule. Those are erased:
     *    after the walk, every cell not on it is set to Empty. That is a
     *    repair rather than a rejection, so generation never has to
     *    reseed for the sake of linearity and never loops unboundedly.
     *
     * @param config What the level is generated from.
     * @return A level whose path is a simple walk from Start to End.
     * @throws LevelError if the config describes a grid too small to
     * hold a path, or if every attempt exhausted its solver budget.
     */
    [[nodiscard]] Level generateLevel(const LevelConfig &config);

} // namespace antwika::tower_defence
