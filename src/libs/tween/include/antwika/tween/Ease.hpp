#pragma once

#include <antwika/animation/Progress.hpp>

#include "antwika/tween/Easing.hpp"

namespace antwika::tween
{

    using antwika::animation::Progress;

    [[nodiscard]] Progress ease(Easing easing, Progress progress);

}
