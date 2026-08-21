#pragma once

#include <antwika/component/Orientation.hpp>
#include <antwika/input/DirectionKeys.hpp>

namespace antwika::rules
{

    inline constexpr float kTurnRate = 0.03F;

    inline constexpr float kMaxPitch = 1.0F;

    [[nodiscard]] component::Orientation rotatedBy(
        component::Orientation orientation, input::DirectionKeys keys);

    [[nodiscard]] component::Orientation turnedBy(
        component::Orientation orientation, float byYaw, float byPitch);

}
