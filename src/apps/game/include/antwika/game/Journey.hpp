#pragma once

#include <optional>

#include <antwika/ecs/Entity.hpp>
#include <antwika/ecs/World.hpp>

#include "antwika/game/Cell.hpp"
#include "antwika/game/GridExtent.hpp"
#include "antwika/game/PathIndex.hpp"

namespace antwika::game
{

    /**
     * @brief Where somebody with no load is walking to, and why.
     *
     * **Errand's counterpart for a person rather than for goods**, and a
     * component of its own rather than a member on that one, because the
     * two answer different questions and a walker never has both.
     * An errand names a building and says what is in the cart; this
     * names either a house somebody is moving into or a way off the map
     * they are taking, and there is nothing in the cart at all.
     *
     * Putting a cell on Errand was tried and rejected: errandTarget()
     * answers with an ecs::Entity, every reader of it treats
     * kNullEntity as "not routed", and a load bound for nowhere is
     * already a state that means something else entirely -- a cart with
     * no store to reach hands its load to the houses it passes.
     * A second meaning for that same value is exactly the kind of thing
     * that shows up as a divergence a long way from its cause.
     *
     * A walker carrying one of these is steered by WalkerSystem toward
     * `towards`; what happens when it gets there is the whole of what
     * `house` says.
     */
    struct Journey
    {
        /**
         * @brief The cell being walked to.
         *
         * A house's origin for somebody moving in, and a road on the
         * edge of the map for somebody leaving.
         * Held even for a mover, rather than looked up off `house` each
         * step, so one arm of WalkerSystem answers for both.
         */
        Cell towards{};

        /**
         * @brief The house being moved into, or kNullEntity to leave.
         *
         * A handle rather than a cell, so a house demolished mid-walk is
         * answered by the world rather than by a stale coordinate that
         * now names bare ground -- exactly as Errand's destination is.
         */
        antwika::ecs::Entity house = antwika::ecs::kNullEntity;

        /**
         * @brief Compare two journeys.
         * @param other The journey to compare against.
         * @return True when both fields match.
         */
        [[nodiscard]] bool operator==(const Journey &other) const = default;
    };

    /**
     * @brief Find the way onto or off the map nearest to a cell.
     *
     * **Where every person in this city comes from and goes to.** A road
     * on the edge of the grid is the whole of what a city border is
     * here: there is nothing beyond the extent, so a road that reaches
     * it is a road that leads somewhere else.
     *
     * Ordered by route length and then by ascending Cell, which is
     * nearestAccepting()'s order and is total for the same reason it
     * needs to be: which gate a migrant uses decides which roads they
     * walk down, and a replay has to pick the same one.
     *
     * @param from The road cell the walk starts or ends at.
     * @param paths The roads a route may run along.
     * @param extent The bounds the search is numbered over, and whose
     * edge is what counts as a way out.
     * @return The border road to use, or nothing when none is reachable
     * -- a city walled off from the outside takes nobody in.
     */
    [[nodiscard]] std::optional<Cell> nearestGate(
        Cell from, const PathIndex &paths, GridExtent extent);

    /**
     * @brief Find the nearest house with room for one more person.
     *
     * The other half of where somebody turned out of a house goes: a
     * neighbour with a spare bed before the road out of town.
     *
     * Ordered exactly as nearestGate() is, and for its reason -- two
     * houses at the same distance is a tie a replay has to break the
     * same way.
     * The house being left is excluded rather than filtered by its
     * occupancy, since a house shedding somebody has room by that very
     * fact and would otherwise take them straight back.
     *
     * @param world The world to read, as of its last commit().
     * @param from The road cell the walk starts at.
     * @param leaving The house being left, which is never the answer.
     * @param paths The roads a route may run along.
     * @param extent The bounds the search is numbered over.
     * @return The house to head for, or kNullEntity when none with room
     * can be reached.
     */
    [[nodiscard]] antwika::ecs::Entity nearestVacancy(
        const antwika::ecs::World &world,
        Cell from,
        antwika::ecs::Entity leaving,
        const PathIndex &paths,
        GridExtent extent);

} // namespace antwika::game
