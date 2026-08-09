#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include <antwika/ecs/Entity.hpp>

#include "antwika/game/BuildingKind.hpp"
#include "antwika/game/Resource.hpp"

namespace antwika::game
{

    inline constexpr std::int32_t kTicksPerSecond = 25;

    inline constexpr std::int32_t kStockCapacity = 100;

    inline constexpr std::int32_t kStockOnCompletion = kStockCapacity / 10;

    inline constexpr std::int32_t kMaxRisk = 100;

    inline constexpr std::int32_t kRiskPeriodTicks = kTicksPerSecond;

    inline constexpr std::int32_t kDrainPeriodTicks = 4 * kTicksPerSecond;

    inline constexpr std::int32_t kMouthsPerServing = 4;

    inline constexpr std::int32_t kSpawnPeriodTicks = kTicksPerSecond;

    inline constexpr std::size_t kMaxWalkersOut = 2;

    struct Building final
    {
        BuildingKind kind = BuildingKind::House;

        std::array<std::int32_t, kResourceCount> stock{
            kStockOnCompletion};

        std::int32_t fireRisk = 0;

        std::int32_t collapseRisk = 0;

        std::int32_t diseaseRisk = 0;

        std::int32_t ticksUntilSpawn = kSpawnPeriodTicks - 1;

        std::int32_t ticksUntilDrain = kDrainPeriodTicks;

        std::int32_t ticksUntilRisk = kRiskPeriodTicks;

        Resource selling = Resource::Food;

        std::array<antwika::ecs::Entity, kMaxWalkersOut> walkers{};

        [[nodiscard]] bool operator==(const Building &other) const = default;
    };

}
