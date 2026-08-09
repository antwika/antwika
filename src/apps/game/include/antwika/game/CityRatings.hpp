#pragma once

#include <cstdint>

#include <antwika/ecs/World.hpp>

namespace antwika::game
{

    using antwika::ecs::World;

    struct CityRatings final
    {
        std::int32_t population = 0;

        std::int32_t employment = 0;

        std::int32_t averageHousingLevel = 0;

        std::int32_t serviceReach = 0;

        [[nodiscard]] constexpr bool operator==(
            const CityRatings &other) const = default;
    };

    [[nodiscard]] CityRatings ratingsOf(const World &world);

}
