#include "antwika/game/PopulationSystem.hpp"

#include <cstddef>
#include <cstdint>
#include <map>
#include <optional>
#include <utility>

#include <antwika/ecs/Entity.hpp>

#include "antwika/game/Building.hpp"
#include "antwika/game/Cell.hpp"
#include "antwika/game/Footprint.hpp"
#include "antwika/game/Household.hpp"
#include "antwika/game/HousingLevel.hpp"
#include "antwika/game/HousingQuery.hpp"
#include "antwika/game/Journey.hpp"
#include "antwika/game/SpawnSystem.hpp"
#include "antwika/game/Walker.hpp"

namespace antwika::game
{

    namespace
    {
        using antwika::ecs::Entity;
        using antwika::ecs::kNullEntity;
    } // namespace

    PopulationSystem::PopulationSystem(
        const PathIndex &paths,
        const BuildingIndex &built,
        const DesirabilityField &desirability,
        GridExtent extent) noexcept
        : paths(paths),
          built(built),
          desirability(desirability),
          extent(extent)
    {
    }

    void PopulationSystem::update(World &world, antwika::time::Tick)
    {
        // Counted before anything is written, and folded in below.
        // A household has exactly one writer in this pass.
        // Two of them would undo each other for HousingSystem's reason.
        const auto arrivals = admit(world);

        for (const auto entity : world.view<Building, Cell>())
        {
            const auto building = world.get<Building>(entity);

            if (!housesPeople(building.kind))
            {
                continue;
            }

            auto household = householdOf(world, entity);

            const auto arriving = arrivals.find(entity);

            if (arriving != arrivals.end())
            {
                // Clamped, since a house may have filled up.
                // Or shrunk, while somebody was walking to it.
                // Whoever cannot get in is gone rather than waiting.
                const auto room = populationCapacityOf(household.level)
                    - household.population;
                household.population +=
                    std::min(arriving->second, std::max(room, 0));
            }

            --household.ticksUntilSettler;

            if (household.ticksUntilSettler <= 0)
            {
                household.ticksUntilSettler = kSettlerPeriodTicks;
                settle(world, entity, building, household);
            }

            setHousehold(world, entity, household);
        }
    }

    void PopulationSystem::settle(
        World &world,
        Entity entity,
        const Building &building,
        Household &household)
    {
        const auto at = world.get<Cell>(entity);
        const auto capacity = populationCapacityOf(household.level);

        // The ground a tier asks for, at the house's own cell.
        // HousingSystem judges it at that very same one.
        const bool pleasant = desirabilityAt(desirability, at)
            >= requirementOf(household.level).desirability;

        // Asked of the block rather than of one cell.
        // And through the very function a spawn uses.
        // So "reachable" means one thing in this application.
        const auto door =
            spawnCellFor(at, footprintOf(building.kind), paths);

        // A house that has just devolved is over its ceiling.
        // Which is the one way to be crowded here.
        if (!pleasant || household.population > capacity)
        {
            if (household.population <= 0)
            {
                return;
            }

            --household.population;
            turnOut(world, entity, door);
            return;
        }

        // Nobody is sent for until the last one has arrived.
        // The handle in the house's own slot is that bookkeeping.
        // A walker destroyed this tick reads alive until commit.
        // So a slot frees up on the next tick, never on this one.
        if (!door.has_value() || household.population >= capacity
            || hasWalkerOfKind(world, building, WalkerKind::Migrant))
        {
            return;
        }

        send(world, entity, building, *door);
    }

    void PopulationSystem::send(
        World &world, Entity entity, const Building &building, Cell door)
    {
        const auto slot = freeWalkerSlot(world, building);
        const auto gate = nearestGate(door, built, extent);
        const auto out = world.view<Walker>().size();

        // A city walled off from the outside takes nobody in.
        // Which is an ordinary answer rather than an error.
        if (!slot.has_value() || !gate.has_value() || out >= kWalkerLimit)
        {
            return;
        }

        const auto migrant = world.create();
        world.add<Cell>(migrant, *gate);
        world.add<Walker>(
            migrant,
            Walker{.kind = WalkerKind::Migrant, .home = entity});
        world.add<Journey>(
            migrant,
            Journey{
                .towards = world.get<Cell>(entity), .house = entity});

        auto expecting = building;
        expecting.walkers[*slot] = migrant;
        world.set<Building>(entity, expecting);
    }

    void PopulationSystem::turnOut(
        World &world, Entity entity, std::optional<Cell> door)
    {
        const auto out = world.view<Walker>().size();

        // Nowhere to walk from, so there is nobody to draw walking.
        // The person has left either way; only the picture differs.
        if (!door.has_value() || out >= kWalkerLimit)
        {
            return;
        }

        // A spare bed in town before the road out of it.
        const auto vacancy =
            nearestVacancy(world, *door, entity, built, extent);

        const auto towards = vacancy != kNullEntity
            ? std::optional<Cell>(world.get<Cell>(vacancy))
            : nearestGate(*door, built, extent);

        if (!towards.has_value())
        {
            return;
        }

        const auto leaver = world.create();
        world.add<Cell>(leaver, *door);

        // Nobody's walker, so no slot and no handle either way.
        // The house they left is not waiting for them to come back.
        world.add<Walker>(leaver, Walker{.kind = WalkerKind::Migrant});
        world.add<Journey>(
            leaver, Journey{.towards = *towards, .house = vacancy});
    }

    std::map<Entity, std::int32_t> PopulationSystem::admit(World &world)
    {
        // Ascending Cell then Entity, out of a map rather than a view.
        // Room in one house is a limited amount two arrivals split.
        // Which is the rule every contended decision here follows.
        // Two migrants may share a cell, so the entity is in the key.
        std::map<std::pair<Cell, Entity>, Entity> arrived;

        for (const auto entity : world.view<Walker, Cell, Journey>())
        {
            const auto journey = world.get<Journey>(entity);

            if (journey.house == kNullEntity
                || !world.has<Cell>(journey.house))
            {
                continue;
            }

            // Beside the block rather than on it.
            // Which is where a walker heading for one actually stops.
            // Nothing stands on a block but the building itself.
            if (!beside(
                    world.get<Cell>(entity),
                    world.get<Cell>(journey.house),
                    footprintOf(
                        world.get<Building>(journey.house).kind)))
            {
                continue;
            }

            arrived.emplace(
                std::pair{world.get<Cell>(entity), entity},
                journey.house);
        }

        std::map<Entity, std::int32_t> counted;

        for (const auto &[key, house] : arrived)
        {
            ++counted[house];
            world.destroy(key.second);
        }

        return counted;
        // The excluded line is the local maps' unwind destructor.
        // Nothing between their construction and the return throws.
    } // GCOVR_EXCL_LINE

} // namespace antwika::game
