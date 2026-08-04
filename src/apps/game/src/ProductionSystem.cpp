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
        // Whether one batch can be finished right now.
        // A workshop out of clay and a full barn are the same answer.
        [[nodiscard]] bool canFinish(
            const Building &building,
            Resource output,
            const Tuning &tuning)
        {
            if (building.stock[resourceIndex(output)]
                >= capacityOf(building.kind))
            {
                return false;
            }

            const auto input = consumedToProduce(building.kind);

            return !input.has_value()
                || building.stock[resourceIndex(*input)]
                       >= tuning.productionBatch;
        }
    } // namespace

    ProductionSystem::ProductionSystem(Tuning tuning) : tuning(tuning)
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

            // Given one rather than skipped.
            // A producer with no component starts a batch from the top.
            // add() is staged, so it takes effect from the next tick.
            if (!world.has<Production>(entity))
            {
                world.add<Production>(
                    entity,
                    Production{
                        .ticksUntilOutput = tuning.productionPeriodTicks});
                continue;
            }

            // Stretched over however few people turned up for work.
            // Nobody at all makes nothing, with the countdown held.
            // Which is the rule a workshop out of clay already follows.
            // Read out here for a second reason.
            const auto period = workedPeriod(
                tuning.productionPeriodTicks, staffingOf(world, entity));

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

            // Held at zero, not reset.
            // So a workshop long out of clay owes nobody a backlog.
            if (!canFinish(building, *output, tuning))
            {
                continue;
            }

            // Copied and adjusted rather than rebuilt.
            // So a member added later is carried across.
            auto worked = building;
            const auto input = consumedToProduce(building.kind);

            if (input.has_value())
            {
                worked.stock[resourceIndex(*input)] -=
                    tuning.productionBatch;
            }

            auto &held = worked.stock[resourceIndex(*output)];
            held = std::min(
                capacityOf(building.kind),
                held + tuning.productionBatch);

            world.set<Building>(entity, worked);
            world.set<Production>(
                entity, Production{.ticksUntilOutput = *period});
        }
    }

} // namespace antwika::game
