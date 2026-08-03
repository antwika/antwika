#pragma once

#include <cstdint>
#include <optional>

#include "antwika/game/BuildingIndex.hpp"
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

    /**
     * @brief Get how far it is to a goal along the roads.
     *
     * stepTowards()'s other half, and the same search: that one answers
     * which way to go and this one answers how far, so nothing outside
     * this file has to build a graph of its own. Whoever chooses between
     * two destinations needs the distance and never the direction, and
     * whoever walks needs the direction and never the distance.
     *
     * **The extent is passed in for the reason stepTowards() gives**,
     * and it matters more here: a caller comparing two routes breaks a
     * tie on the goal, so a NodeId numbered over a bounding box of
     * whichever roads happen to exist would move the answer every time a
     * road was laid somewhere else entirely.
     *
     * @param from Where the walker is; must be a road to get anywhere.
     * @param goal The minimum-x, minimum-y cell of where it is heading.
     * @param footprint How many cells across and down the goal covers.
     * @param paths The roads a route may run along.
     * @param extent The bounds the search is numbered over.
     * @return How many steps the route takes, or nullopt when there is
     * none -- the same ordinary answer stepTowards() gives, covering a
     * walled-off goal, a demolished road and a cell outside the extent.
     */
    [[nodiscard]] std::optional<std::int64_t> routeCost(
        Cell from,
        Cell goal,
        Footprint footprint,
        const PathIndex &paths,
        GridExtent extent);

    /**
     * @brief Get which way to step to get closer to a goal, over open
     * ground.
     *
     * stepTowards()'s sibling, and it is the same search with one thing
     * changed: what a route may run along.
     * That one is bound to the roads, because a water carrier, a cart
     * pusher and a market seller are all doing the city's business and
     * the road network is what that business runs on.
     * This one is bound only by what is standing: everything inside the
     * extent is walkable except the cells a building covers.
     *
     * **A person is not a delivery, which is the whole distinction.**
     * Somebody moving house walks where they like -- across a field,
     * round the back of a workshop, straight at the edge of the map --
     * and being made to follow a road nobody built yet is what kept a
     * city with no way out of it from taking anybody in.
     *
     * Every cell of the goal's block is walkable by exception, exactly
     * as it is over the roads, since a walker arrives by stepping onto
     * what it was heading for.
     *
     * Replay-safe on stepTowards()' terms, and for its reasons: the
     * open set orders down to ascending NodeId, and the extent is
     * passed in rather than derived so that numbering cannot move when
     * something is built somewhere else entirely.
     *
     * @param from Where the walker is.
     * @param goal The minimum-x, minimum-y cell of where it is heading.
     * @param footprint How many cells across and down the goal covers.
     * @param built The buildings a route may not cross.
     * @param extent The bounds the search is numbered over.
     * @return The direction of the first step, or nullopt when there is
     * no route -- an ordinary answer covering a goal walled in by
     * buildings, a cell outside the extent and a degenerate extent.
     */
    [[nodiscard]] std::optional<Direction> stepAcross(
        Cell from,
        Cell goal,
        Footprint footprint,
        const BuildingIndex &built,
        GridExtent extent);

    /**
     * @brief Get how far it is to a goal over open ground.
     *
     * stepAcross()'s other half, on exactly routeCost()'s terms: that
     * pair answers along the roads and this pair answers across the
     * ground, and each is one search asked two questions so that
     * nothing outside this file builds a graph of its own.
     *
     * @param from Where the walker is.
     * @param goal The minimum-x, minimum-y cell of where it is heading.
     * @param footprint How many cells across and down the goal covers.
     * @param built The buildings a route may not cross.
     * @param extent The bounds the search is numbered over.
     * @return How many steps the route takes, or nullopt when there is
     * none.
     */
    [[nodiscard]] std::optional<std::int64_t> crossingCost(
        Cell from,
        Cell goal,
        Footprint footprint,
        const BuildingIndex &built,
        GridExtent extent);

} // namespace antwika::game
