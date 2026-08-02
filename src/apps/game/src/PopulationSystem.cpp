#include "antwika/game/PopulationSystem.hpp"

#include <cstdint>

#include <antwika/ecs/Entity.hpp>

#include "antwika/game/Building.hpp"
#include "antwika/game/Cell.hpp"
#include "antwika/game/Footprint.hpp"
#include "antwika/game/Household.hpp"
#include "antwika/game/HousingLevel.hpp"
#include "antwika/game/HousingQuery.hpp"
#include "antwika/game/SpawnSystem.hpp"

namespace antwika::game
{

    PopulationSystem::PopulationSystem(
        const PathIndex &paths,
        const DesirabilityField &desirability) noexcept
        : paths(paths), desirability(desirability)
    {
    }

    void PopulationSystem::update(World &world, antwika::time::Tick)
    {
        for (const auto entity : world.view<Building, Cell>())
        {
            const auto building = world.get<Building>(entity);

            if (!housesPeople(building.kind))
            {
                continue;
            }

            auto household = householdOf(world, entity);
            --household.ticksUntilSettler;

            if (household.ticksUntilSettler <= 0)
            {
                household.ticksUntilSettler = kSettlerPeriodTicks;

                const auto at = world.get<Cell>(entity);
                const auto capacity =
                    populationCapacityOf(household.level);

                // The ground a tier asks for, at the house's own cell.
                // HousingSystem judges it at that very same one.
                const bool pleasant =
                    desirabilityAt(desirability, at)
                    >= requirementOf(household.level).desirability;

                // Asked of the block rather than of one cell.
                // And through the very function a spawn uses.
                // So "reachable" means one thing in this application.
                const bool reachable =
                    spawnCellFor(at, footprintOf(building.kind), paths)
                        .has_value();

                // A house that has just devolved is over its ceiling.
                // Which is the one way to be crowded here.
                if (!pleasant || household.population > capacity)
                {
                    household.population =
                        household.population > 0
                            ? household.population - 1
                            : 0;
                }
                else if (reachable && household.population < capacity)
                {
                    ++household.population;
                }
            }

            setHousehold(world, entity, household);
        }
    }

} // namespace antwika::game
