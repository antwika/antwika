#include "antwika/game/HousingSystem.hpp"

#include <cstddef>

#include <antwika/ecs/Entity.hpp>

#include "antwika/game/Building.hpp"
#include "antwika/game/Cell.hpp"
#include "antwika/game/Coverage.hpp"
#include "antwika/game/Household.hpp"
#include "antwika/game/HousingLevel.hpp"
#include "antwika/game/Resource.hpp"
#include "antwika/game/Service.hpp"
#include "antwika/game/Store.hpp"

namespace antwika::game
{

    namespace
    {
        using antwika::ecs::Entity;

        [[nodiscard]] bool meets(
            const World &world,
            const DesirabilityField &field,
            Entity entity,
            Cell at,
            HousingLevel level)
        {
            const auto wanted = requirementOf(level);

            if (desirabilityAt(field, at) < wanted.desirability)
            {
                return false;
            }

            for (const auto service : kServices)
            {
                if (wanted.services[serviceIndex(service)]
                    && coverageOf(world, entity, service) <= 0)
                {
                    return false;
                }
            }

            for (const auto resource : kResources)
            {
                if (stockOf(world, entity, resource)
                    < wanted.goods[resourceIndex(resource)])
                {
                    return false;
                }
            }

            return true;
        }

        void settleAt(
            Household &household,
            std::size_t index,
            const GameConfig &config)
        {
            household.level = static_cast<HousingLevel>(index);
            household.ticksUntilEvolve = config.evolvePeriodTicks;
            household.ticksUntilDevolve = config.devolvePeriodTicks;
        }
    }

    HousingSystem::HousingSystem(
        const DesirabilityField &desirability, GameConfig config)
        : desirability(desirability), config(config)
    {
    }

    void HousingSystem::update(World &world, antwika::time::Tick)
    {
        for (const auto entity : world.view<Building, Cell>())
        {
            if (!housesPeople(world.get<Building>(entity).kind))
            {
                continue;
            }

            const auto at = world.get<Cell>(entity);
            auto household = householdOf(world, entity);
            const auto index = housingLevelIndex(household.level);
            const auto next = index + 1;

            if (next < kHousingLevelCount
                && meets(
                    world,
                    desirability,
                    entity,
                    at,
                    static_cast<HousingLevel>(next)))
            {
                household.ticksUntilDevolve = config.devolvePeriodTicks;
                --household.ticksUntilEvolve;

                if (household.ticksUntilEvolve <= 0)
                {
                    settleAt(household, next, config);
                }
            }
            else if (
                index > 0
                && !meets(
                    world, desirability, entity, at, household.level))
            {
                household.ticksUntilEvolve = config.evolvePeriodTicks;
                --household.ticksUntilDevolve;

                if (household.ticksUntilDevolve <= 0)
                {
                    settleAt(household, index - 1, config);
                }
            }
            else
            {
                household.ticksUntilEvolve = config.evolvePeriodTicks;
                household.ticksUntilDevolve = config.devolvePeriodTicks;
            }

            if (household == Household{}
                && !world.has<Household>(entity))
            {
                continue;
            }

            setHousehold(world, entity, household);
        }
    }

}
