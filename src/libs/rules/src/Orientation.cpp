#include "antwika/rules/Orientation.hpp"

#include <algorithm>
#include <antwika/component/Orientation.hpp>

namespace antwika::rules
{

    component::Orientation turnedBy(
        const component::Orientation orientation,
        const float byYaw,
        const float byPitch)
    {
        return component::Orientation{
            .yaw = orientation.yaw + byYaw,
            .pitch = std::clamp(
                orientation.pitch + byPitch,
                -kMaxPitch,
                kMaxPitch)};
    }

    component::Orientation rotatedBy(
        const component::Orientation orientation,
        const component::TurnIntent intent)
    {
        return turnedBy(
            orientation,
            intent.axisX * kTurnRate,
            intent.axisZ * kTurnRate);
    }

}
