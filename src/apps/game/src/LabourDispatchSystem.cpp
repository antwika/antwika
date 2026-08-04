#include "antwika/game/LabourDispatchSystem.hpp"

#include <cstdint>
#include <map>

#include <antwika/ecs/Entity.hpp>

#include "antwika/game/Building.hpp"
#include "antwika/game/Cell.hpp"
#include "antwika/game/Employment.hpp"
#include "antwika/game/Footprint.hpp"
#include "antwika/game/Household.hpp"
#include "antwika/game/HousingLevel.hpp"
#include "antwika/game/SpawnSystem.hpp"
#include "antwika/game/Walker.hpp"

namespace antwika::game
{

    namespace
    {
        using antwika::ecs::Entity;
    } // namespace

    LabourDispatchSystem::LabourDispatchSystem(
        const PathIndex &paths, Tuning tuning) noexcept
        : paths(paths), tuning(tuning)
    {
    }

    void LabourDispatchSystem::update(World &world, antwika::time::Tick)
    {
        // Ascending Cell, since the walker limit is a split amount.
        // No tie-break: BuildingIndex keeps two houses off one cell.
        std::map<Cell, Entity> houses;

        for (const auto entity : world.view<Building, Cell>())
        {
            if (housesPeople(world.get<Building>(entity).kind))
            {
                houses.emplace(world.get<Cell>(entity), entity);
            }
        }

        // Committed walkers plus the ones sent below, counted by hand.
        auto out = world.view<Walker>().size();

        for (const auto &[at, entity] : houses)
        {
            const auto building = world.get<Building>(entity);
            const auto household = householdOf(world, entity);

            auto employment = world.has<Employment>(entity)
                ? world.get<Employment>(entity)
                : Employment{};

            const auto idle =
                household.population - employedCount(employment);

            // Everybody is working, so there is nothing to carry.
            // The countdown holds rather than running.
            // So a house owes no walker the moment somebody is free.
            if (idle <= 0)
            {
                continue;
            }

            if (employment.ticksUntilDispatch > 0)
            {
                --employment.ticksUntilDispatch;
                setEmployment(world, entity, employment);
                continue;
            }

            const auto door =
                spawnCellFor(at, footprintOf(building.kind), paths);
            const auto slot = freeWalkerSlot(world, building);

            // One labourer out at a time, exactly one walker per kind.
            // The cadence rule every sender here already lives under.
            if (!door.has_value() || !slot.has_value()
                || hasWalkerOfKind(world, building, WalkerKind::Labourer)
                || out >= tuning.walkerLimit)
            {
                continue;
            }

            employment.ticksUntilDispatch = tuning.labourPeriodTicks;
            setEmployment(world, entity, employment);

            const auto labourer = world.create();
            world.add<Cell>(labourer, *door);
            world.add<Walker>(
                labourer,
                Walker{
                    .kind = WalkerKind::Labourer,
                    .carried = idle,
                    .home = entity});

            auto expecting = building;
            expecting.walkers[*slot] = labourer;
            world.set<Building>(entity, expecting);

            ++out;
        }
    }

} // namespace antwika::game
