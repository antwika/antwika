#include "antwika/game/ProductionSystem.hpp"

#include <algorithm>

#include "antwika/game/Building.hpp"
#include "antwika/game/Cell.hpp"
#include "antwika/game/LabourQuery.hpp"
#include "antwika/game/Production.hpp"
#include "antwika/game/Resource.hpp"
#include "antwika/game/Store.hpp"

namespace antwika::game
{

    namespace
    {
        [[nodiscard]] bool canFinish(
            const Building &building,
            Resource output,
            const GameConfig &config)
        {
            if (building.stock[resourceIndex(output)]
                >= capacityOf(building.kind))
            {
                return false;
            }

            const auto input = consumedToProduce(building.kind);

            return !input.has_value()
                || building.stock[resourceIndex(*input)]
                       >= config.productionBatch;
        }
    }

    ProductionSystem::ProductionSystem(GameConfig config) : config(config)
    {
    }

    void ProductionSystem::update(World &world, antwika::time::Tick)
    {
        for (const auto entity : world.view<Building, Cell>())
        {
            const auto building = world.get<Building>(entity);
            const auto output = producedBy(building.kind);

            if (!output.has_value())
            {
                continue;
            }

            if (!world.has<Production>(entity))
            {
                world.add<Production>(
                    entity,
                    Production{
                        .ticksUntilOutput = config.productionPeriodTicks});
                continue;
            }

            const auto period = workedPeriod(
                config.productionPeriodTicks, staffingOf(world, entity));

            if (!period.has_value())
            {
                continue;
            }

            const auto production = world.get<Production>(entity);

            if (production.ticksUntilOutput > 0)
            {
                world.set<Production>(
                    entity,
                    Production{
                        .ticksUntilOutput =
                            production.ticksUntilOutput - 1});
                continue;
            }

            if (!canFinish(building, *output, config))
            {
                continue;
            }

            auto worked = building;
            const auto input = consumedToProduce(building.kind);

            if (input.has_value())
            {
                worked.stock[resourceIndex(*input)] -=
                    config.productionBatch;
            }

            auto &held = worked.stock[resourceIndex(*output)];
            held = std::min(
                capacityOf(building.kind),
                held + config.productionBatch);

            world.set<Building>(entity, worked);
            world.set<Production>(
                entity, Production{.ticksUntilOutput = *period});
        }
    }

}
