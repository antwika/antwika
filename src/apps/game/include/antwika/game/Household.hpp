#pragma once

#include <cstdint>

#include <antwika/ecs/Entity.hpp>
#include <antwika/ecs/World.hpp>

#include "antwika/game/HousingLevel.hpp"

namespace antwika::game
{

    using antwika::ecs::World;

    inline constexpr std::int32_t kMigrantPeriodTicks =
        kTicksPerSecond / 4;

    struct Household final
    {
        HousingLevel level = HousingLevel::Tent;

        std::int32_t ticksUntilEvolve = kEvolvePeriodTicks;

        std::int32_t ticksUntilDevolve = kDevolvePeriodTicks;

        std::int32_t population = 0;

        [[nodiscard]] bool operator==(const Household &other) const
            = default;
    };

    [[nodiscard]] Household householdOf(
        const World &world, antwika::ecs::Entity entity);

    void setHousehold(
        World &world, antwika::ecs::Entity entity, Household household);

}
