#pragma once

#include <cstdint>
#include <map>
#include <optional>

#include <antwika/ecs/Entity.hpp>
#include <antwika/ecs/ISystem.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/time/Tick.hpp>

#include "antwika/game/Building.hpp"
#include "antwika/game/BuildingIndex.hpp"
#include "antwika/game/Cell.hpp"
#include "antwika/game/Desirability.hpp"
#include "antwika/game/GridExtent.hpp"
#include "antwika/game/Household.hpp"
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
     * **Nobody appears out of thin air and nobody vanishes into it.**
     * A house that is due somebody sends for them, and what arrives is
     * an ordinary walker carrying a Journey: it enters at the nearest
     * road on the edge of the map, walks the roads to the door, and the
     * house's number goes up on the tick it gets there rather than on
     * the tick it was sent for. So a district a long way from any gate
     * fills more slowly than one beside it, which is a fact about the
     * road network rather than a rule written anywhere.
     *
     * A house that sheds somebody sends one out the same way, and where
     * they go is decided in that order: the nearest house with a bed
     * going, and failing that the nearest road out of town. A house
     * that has just devolved therefore does not evaporate its overflow
     * -- the people it can no longer hold walk to whatever room there
     * is, and the city's total only falls when there is none.
     *
     * **A house asks for nobody while somebody is on the way**, and the
     * bookkeeping for that is the handle in its own walker slot rather
     * than a flag of its own: a house is already the thing that knows
     * which walkers it has out, and ecs::EntityManager never reusing an
     * index is already why a stale handle can only be dead.
     * Somebody walking *out* takes no slot, because the house they left
     * is not waiting for them.
     *
     * **There is still no shared migrant pool**: every house is bounded
     * by its own capacity and asks for nobody anybody else is asking
     * for, so sending is an independent per-entity effect and reads
     * ecs::View directly. Arriving is not, because two people reaching
     * one house on one tick split the room it has left, so the arrivals
     * are walked out of a std::map keyed by ascending cell and entity --
     * the entity being in the key because two walkers may share a cell.
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
         * @param built The blocks a person crossing open ground may not
         * walk through; must outlive this system.
         * @param desirability The field, read at each house's own origin
         * cell; must outlive this system.
         */
        PopulationSystem(
            const PathIndex &paths,
            const BuildingIndex &built,
            const DesirabilityField &desirability,
            GridExtent extent) noexcept;

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
        // Whoever reached the house they were walking to, per house.
        // The walkers are retired here; the counting is the caller's.
        [[nodiscard]] std::map<antwika::ecs::Entity, std::int32_t> admit(
            World &world);

        void settle(
            World &world,
            antwika::ecs::Entity entity,
            const Building &building,
            Household &household);

        void send(
            World &world,
            antwika::ecs::Entity entity,
            const Building &building,
            Cell door);

        void turnOut(
            World &world,
            antwika::ecs::Entity entity,
            std::optional<Cell> door);

        const PathIndex &paths;
        const BuildingIndex &built;
        const DesirabilityField &desirability;
        GridExtent extent;
    };

} // namespace antwika::game
