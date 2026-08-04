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

        // Everything one level asks of one house, in one answer.
        // The ground is judged at the house's own origin cell.
        // Rather than averaged over its block, since a house is a cell.
        // An average is a number nobody could point at.
        // Coverage is asked whether it reaches at all.
        // Never how much of it is left.
        // Its whole job is to say whether somebody came recently.
        // A threshold on it would be a second, unstated period.
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

        // Both countdowns, whichever way the house just went.
        // A level is arrived at fresh rather than part-way out of it.
        void settleAt(
            Household &household,
            std::size_t index,
            const Tuning &tuning)
        {
            household.level = static_cast<HousingLevel>(index);
            household.ticksUntilEvolve = tuning.evolvePeriodTicks;
            household.ticksUntilDevolve = tuning.devolvePeriodTicks;
        }
    } // namespace

    HousingSystem::HousingSystem(
        const DesirabilityField &desirability, Tuning tuning)
        : desirability(desirability), tuning(tuning)
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

            // The two arms cannot both apply, and the table says why.
            // Every demand rises with the level.
            // See kHousingRequirements' own static_assert.
            // So meeting the next row implies meeting this one.
            // A house owed a promotion is never also owed a demotion.
            if (next < kHousingLevelCount
                && meets(
                    world,
                    desirability,
                    entity,
                    at,
                    static_cast<HousingLevel>(next)))
            {
                household.ticksUntilDevolve = tuning.devolvePeriodTicks;
                --household.ticksUntilEvolve;

                if (household.ticksUntilEvolve <= 0)
                {
                    settleAt(household, next, tuning);
                }
            }
            else if (
                index > 0
                && !meets(
                    world, desirability, entity, at, household.level))
            {
                household.ticksUntilEvolve = tuning.evolvePeriodTicks;
                --household.ticksUntilDevolve;

                if (household.ticksUntilDevolve <= 0)
                {
                    settleAt(household, index - 1, tuning);
                }
            }
            else
            {
                // Reset rather than held where they were.
                // "Has had this for a while" is what is measured.
                // One that remembered a broken stretch would not.
                // It would measure "has had this on and off".
                household.ticksUntilEvolve = tuning.evolvePeriodTicks;
                household.ticksUntilDevolve = tuning.devolvePeriodTicks;
            }

            // Given one only once there is something to say.
            // A default household and no component mean one thing.
            // So a house that has never done anything acquires neither.
            // Which is the rule Coverage states at length.
            if (household == Household{}
                && !world.has<Household>(entity))
            {
                continue;
            }

            setHousehold(world, entity, household);
        }
    }

} // namespace antwika::game
