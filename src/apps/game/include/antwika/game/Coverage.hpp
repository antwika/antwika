#pragma once

#include <array>
#include <cstdint>

#include <antwika/ecs/Entity.hpp>
#include <antwika/ecs/World.hpp>

#include "antwika/game/Building.hpp"
#include "antwika/game/Service.hpp"

namespace antwika::game
{

    using antwika::ecs::World;

    inline constexpr std::int32_t kCoverageFull = 20 * kTicksPerSecond;

    struct Coverage final
    {
        std::array<std::int32_t, kServiceCount> ticksLeft{};

        [[nodiscard]] bool operator==(const Coverage &other) const = default;
    };

    [[nodiscard]] Coverage coverageOf(
        const World &world, antwika::ecs::Entity entity);

    [[nodiscard]] std::int32_t coverageOf(
        const World &world,
        antwika::ecs::Entity entity,
        Service service);

    void setCoverage(
        World &world, antwika::ecs::Entity entity, Coverage coverage);

}
