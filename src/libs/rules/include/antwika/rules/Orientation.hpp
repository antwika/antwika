#pragma once

#include <antwika/component/Orientation.hpp>
#include <antwika/component/TurnIntent.hpp>

namespace antwika::rules
{

    inline constexpr float kTurnRate = 0.03F;

    inline constexpr float kMaxPitch = 1.0F;

    [[nodiscard]] component::Orientation rotatedBy(
        component::Orientation orientation,
        component::TurnIntent intent);

    [[nodiscard]] component::Orientation turnedBy(
        component::Orientation orientation, float byYaw, float byPitch);

}
