#pragma once

#include <antwika/ecs/Entity.hpp>
#include <antwika/ecs/World.hpp>

#include "antwika/game/Cell.hpp"
#include "antwika/game/GridExtent.hpp"
#include "antwika/game/PathIndex.hpp"
#include "antwika/game/Resource.hpp"

namespace antwika::game
{

    /**
     * @brief Find the nearest store with room for a load.
     *
     * **The total order this breaks down to is path length, then
     * ascending Cell of the store's origin**, and both halves are load
     * bearing. The length comes from antwika::pathfinding, whose open
     * set orders down to ascending NodeId, so an equal-cost route
     * resolves the same way on every run and every toolchain -- and the
     * graph is numbered over the *configured* extent, never a bounding
     * box of the roads, because a bounding box renumbers every node as a
     * road is laid and takes the tie-break with it.
     * The second half needs no tie-break of its own: BuildingIndex
     * guarantees two buildings cannot share an origin cell, which is the
     * strongest form of a total order available here.
     *
     * A store already full of that resource is not chosen, so a cart is
     * never sent somewhere it would have to turn round.
     *
     * @param world The world to read, as of its last commit().
     * @param from The road cell the cart would set out from.
     * @param resource What is in the cart.
     * @param paths The roads a route may run along.
     * @param extent The bounds the search is numbered over.
     * @return The store to head for, or kNullEntity when no store with
     * room can be reached -- an ordinary answer meaning the load stays
     * with the walker and goes round with it.
     */
    [[nodiscard]] antwika::ecs::Entity nearestAccepting(
        const antwika::ecs::World &world,
        Cell from,
        Resource resource,
        const PathIndex &paths,
        GridExtent extent);

    /**
     * @brief Find the nearest store holding any of a resource.
     *
     * nearestAccepting()'s mirror, ordered identically, and the one a
     * market's buyer is sent along.
     *
     * @param world The world to read, as of its last commit().
     * @param from The road cell the buyer would set out from.
     * @param resource What the buyer is after.
     * @param paths The roads a route may run along.
     * @param extent The bounds the search is numbered over.
     * @return The store to head for, or kNullEntity when none holding
     * it can be reached, in which case no buyer is worth sending.
     */
    [[nodiscard]] antwika::ecs::Entity nearestHolding(
        const antwika::ecs::World &world,
        Cell from,
        Resource resource,
        const PathIndex &paths,
        GridExtent extent);

} // namespace antwika::game
