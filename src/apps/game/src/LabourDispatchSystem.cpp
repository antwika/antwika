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
    }

    LabourDispatchSystem::LabourDispatchSystem(
        const PathIndex &paths, GameConfig config) noexcept
        : paths(paths), config(config)
    {
    }

    void LabourDispatchSystem::update(World &world, antwika::time::Tick)
    {
        std::map<Cell, Entity> houses;

        for (const auto entity : world.view<Building, Cell>())
        {
            if (housesPeople(world.get<Building>(entity).kind))
            {
                houses.emplace(world.get<Cell>(entity), entity);
            }
        }

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

            if (!door.has_value() || !slot.has_value()
                || hasWalkerOfKind(world, building, WalkerKind::Labourer)
                || out >= config.walkerLimit)
            {
                continue;
            }

            employment.ticksUntilDispatch = config.labourPeriodTicks;
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

}
