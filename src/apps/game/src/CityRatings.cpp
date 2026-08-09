#include "antwika/game/CityRatings.hpp"

#include <cstdint>

#include <antwika/ecs/Entity.hpp>

#include "antwika/game/Building.hpp"
#include "antwika/game/Cell.hpp"
#include "antwika/game/Coverage.hpp"
#include "antwika/game/HousingLevel.hpp"
#include "antwika/game/HousingQuery.hpp"
#include "antwika/game/LabourQuery.hpp"
#include "antwika/game/Service.hpp"

namespace antwika::game
{

    namespace
    {
        [[nodiscard]] std::int32_t shareOf(
            std::int32_t part, std::int32_t whole, std::int32_t scale)
        {
            if (whole <= 0)
            {
                return 0;
            }

            return part * scale / whole;
        }
    }

    CityRatings ratingsOf(const World &world)
    {
        std::int32_t population = 0;
        std::int32_t staffed = 0;
        std::int32_t jobs = 0;
        std::int32_t houses = 0;
        std::int32_t tiers = 0;
        std::int32_t reached = 0;

        for (const auto entity : world.view<Building, Cell>())
        {
            const auto staffing = staffingOf(world, entity);
            staffed += staffing.filled;
            jobs += staffing.wanted;

            if (!housesPeople(world.get<Building>(entity).kind))
            {
                continue;
            }

            ++houses;
            population += populationAt(world, entity);
            tiers += static_cast<std::int32_t>(
                housingLevelIndex(levelOf(world, entity)));

            for (const auto service : kServices)
            {
                if (coverageOf(world, entity, service) > 0)
                {
                    ++reached;
                }
            }
        }

        return CityRatings{
            .population = population,
            .employment = shareOf(staffed, jobs, 100),
            .averageHousingLevel = shareOf(tiers, houses, 100),
            .serviceReach = shareOf(
                reached,
                houses * static_cast<std::int32_t>(kServiceCount),
                100)};
    }

}
