#pragma once

#include <cstdint>

namespace antwika::component
{

    inline constexpr float kPatrolArrivalRadius = 0.15F;

    inline constexpr float kStrollSpeedFactor = 0.5F;

    inline constexpr std::uint64_t kMaxPatrolSteps = 20000;

    struct Patrol final
    {
        std::uint32_t nextStopIndex = 0;

        std::uint32_t pathIndex = 0;
    };

}
