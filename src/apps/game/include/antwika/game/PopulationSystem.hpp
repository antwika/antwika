#pragma once

#include <antwika/ecs/ISystem.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/time/Tick.hpp>

#include "antwika/game/Desirability.hpp"
#include "antwika/game/PathIndex.hpp"

namespace antwika::game
{

    using antwika::ecs::ISystem;
    using antwika::ecs::World;

    /**
     * @brief Moves people into the houses worth living in, and out of the
     * ones that are not.
     *
     * Three conditions, all of them already simulation state. A house
     * needs **a road beside it**, because everybody in this city arrives
     * and leaves on foot and a block nothing can reach is a block nobody
     * moves into. It needs the **desirability its own tier asks for**,
     * read off the field at its origin cell, which is what makes a clay
     * pit next door cost a district its people. And it needs **room**,
     * which populationCapacityOf() answers from the tier a house has
     * grown to -- so raising a district's housing is what raises its
     * population, and that is the loop the whole application is arranged
     * around.
     *
     * One person at a time, on the house's own kSettlerPeriodTicks
     * countdown. A house that is crowded -- which is what a house that
     * has just devolved is -- or standing on ground below its tier's
     * threshold loses one instead, down to nobody and no further.
     *
     * **There is no shared migrant pool, deliberately.** Every house is
     * bounded by its own capacity and takes from nothing anybody else is
     * taking from, so this is an independent per-entity effect and reads
     * ecs::View directly. Labour is the increment's one contended
     * allocation and it is LabourSystem's, where an order over the
     * workplaces is named -- see LabourSystem.
     *
     * **It runs in a phase after HousingSystem's rather than beside it,
     * and that is load-bearing.** A phase is where the World's buffers
     * swap: two systems in one phase both read what the last swap left
     * and both write a whole Household back, so the tier one of them
     * wrote and the occupancy the other wrote could not both survive.
     * The later write would silently undo the earlier, some of the time,
     * and a divergent replay a long way from its cause is what that looks
     * like. The commit between the two phases is what makes them
     * sequential.
     *
     * **Nothing here is a persisted event.** The candidate was
     * `game.immigrant_arrived`, which reads like a notification worth
     * recording; it is a pure function of a road, a field and a tier, all
     * of which a replay regenerates from the clicks that built them, so
     * recording it would house the same person twice.
     */
    class PopulationSystem final : public ISystem
    {
    public:
        /**
         * @brief Construct the system over what a house is judged by.
         * @param paths The roads, asked whether one runs beside a house;
         * must outlive this system.
         * @param desirability The field, read at each house's own origin
         * cell; must outlive this system.
         */
        PopulationSystem(
            const PathIndex &paths,
            const DesirabilityField &desirability) noexcept;

        PopulationSystem(const PopulationSystem &) = delete;
        PopulationSystem(PopulationSystem &&) = delete;

        PopulationSystem &operator=(const PopulationSystem &) = delete;
        PopulationSystem &operator=(PopulationSystem &&) = delete;

        /**
         * @brief Move one person in or out of every house that is due.
         * @param world Read for the houses, staged into with their
         * households.
         * @param tick The tick being processed; unused, because each
         * house counts down in its own component rather than off a
         * modulus of it -- see Household.
         */
        void update(World &world, antwika::time::Tick tick) override;

    private:
        const PathIndex &paths;
        const DesirabilityField &desirability;
    };

} // namespace antwika::game
