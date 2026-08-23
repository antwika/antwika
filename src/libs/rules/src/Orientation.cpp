#include "antwika/rules/Orientation.hpp"

#include <algorithm>
#include <antwika/component/Orientation.hpp>

namespace antwika::rules
{

    component::Orientation getTurnedBy(
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

    component::Orientation getRotatedBy(
        const component::Orientation orientation,
        const component::TurnIntent intent)
    {
        return getTurnedBy(
            orientation,
            intent.axisX * kTurnRate,
            intent.axisZ * kTurnRate);
    }

}
