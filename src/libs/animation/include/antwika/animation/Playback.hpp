#pragma once

#include <antwika/time/Tick.hpp>

#include "antwika/animation/Clip.hpp"
#include "antwika/animation/Frame.hpp"
#include "antwika/animation/Progress.hpp"

namespace antwika::animation
{

    [[nodiscard]] Frame getFrameAt(const Clip &clip, time::Tick elapsedTicks);

    [[nodiscard]] Progress getStepProgress(
        time::Tick elapsedTicks, time::Tick ticksPerStep);

}
