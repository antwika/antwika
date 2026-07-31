#pragma once

#include <optional>

#include "antwika/game/Cell.hpp"
#include "antwika/game/Direction.hpp"
#include "antwika/game/Footprint.hpp"
#include "antwika/game/GridExtent.hpp"
#include "antwika/game/PathIndex.hpp"

namespace antwika::game
{

    /**
     * @brief Get which way to step to get closer to a goal cell.
     *
     * An A* over the roads, run once per step rather than once per
     * journey, with only its first move used.
     * No route is stored anywhere, and that is deliberate: a route
     * cannot live in a component, because ecs::Component forbids a
     * vector and a fixed-size one would cap the map; and a route held
     * in a system is state outside the World that a save does not
     * cover and that would need invalidating every time a road changed.
     * Re-searching costs one pass over the roads per walker per step,
     * and the caching version is a strict optimisation behind this same
     * signature if it ever bites.
     *
     * **Replay-safe by construction.** antwika::pathfinding orders its
     * open set down to ascending NodeId, so no two entries ever compare
     * equal and an equal-cost route resolves the same way on every run
     * and every toolchain.
     * The extent is passed in rather than derived from the roads for
     * exactly that reason: NodeId here is the row-major index over the
     * extent, so a bounding box computed from whichever roads happen to
     * exist would renumber every node as one was laid, and with it the
     * tie-break.
     *
     * Every cell of the goal's block is passable by exception, since a
     * building does not stand on a road and would otherwise be
     * unreachable.
     * The search heads for the block's origin, which is its
     * lexicographic minimum and therefore a stable choice whichever
     * corner a walker happens to approach from.
     *
     * @param from Where the walker is; must be a road to get anywhere.
     * @param goal The minimum-x, minimum-y cell of where it is heading.
     * @param footprint How many cells across and down the goal covers.
     * @param paths The roads a route may run along.
     * @param extent The bounds the search is numbered over.
     * @return The direction of the first step, or nullopt when there is
     * no route -- which is an ordinary answer covering a walled-off
     * goal, a demolished road, a goal or start outside the extent, and
     * a degenerate extent, rather than an error in any of those cases.
     */
    [[nodiscard]] std::optional<Direction> stepTowards(
        Cell from,
        Cell goal,
        Footprint footprint,
        const PathIndex &paths,
        GridExtent extent);

} // namespace antwika::game
