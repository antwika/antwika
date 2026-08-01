#pragma once

#include <vector>

#include "antwika/game/BuildingIndex.hpp"
#include "antwika/game/Cell.hpp"
#include "antwika/game/GridExtent.hpp"

namespace antwika::game
{

    /**
     * @brief The run of cells a road drag would lay, and whether it may.
     *
     * **The cells are what to show, and `valid` alone says what to
     * build.** A refused plan still carries the two cells somebody named,
     * so the refusal can be drawn reddened where they named it -- the
     * convention canPlace() and the build ghost already follow, since a
     * refusal shown is one somebody can act on and a preview that
     * vanishes leaves them guessing. Nothing may be laid from a plan
     * whose `valid` is false.
     */
    struct RoadPlan
    {
        /**
         * @brief The route, in order, or the two named cells when there
         * is none.
         *
         * Empty when there is nothing to say at all, which is a drag that
         * named a cell off the grid.
         */
        std::vector<Cell> cells;

        /** @brief Whether a route was found, and so whether to lay it. */
        bool valid = false;

        /**
         * @brief Compare two plans.
         * @param other The plan to compare against.
         * @return True when both the cells and the verdict match.
         */
        [[nodiscard]] bool operator==(const RoadPlan &other) const = default;
    };

    /**
     * @brief Work out the run of road between two cells.
     *
     * An A* through antwika::pathfinding, on exactly stepTowards()'s
     * terms and for exactly its reason: the search orders ties down to
     * ascending NodeId, so an equal-cost route resolves the same way on
     * every run and every toolchain -- which is the only reason a replay
     * may depend on a route at all.
     *
     * **The extent is passed in rather than derived from what happens to
     * exist**, which is the load-bearing half of that. A bounding box
     * taken off the roads or the buildings would renumber every node as
     * one was laid, and the tie-break with it, so two runs of one
     * recording would plan two different routes.
     *
     * A road already laid is passable and is simply not laid again, so
     * the roads are not consulted here at all: what a route may not cross
     * is a building. That also keeps the plan a function of strictly less
     * state than the placement it feeds.
     *
     * @param from The cell the drag started on.
     * @param to The cell the pointer is over now.
     * @param extent The bounds a route must stay inside.
     * @param built The buildings a route may not cross.
     * @return The route to lay, or a refused plan naming the two cells.
     */
    [[nodiscard]] RoadPlan planRoad(
        Cell from, Cell to, GridExtent extent, const BuildingIndex &built);

} // namespace antwika::game
