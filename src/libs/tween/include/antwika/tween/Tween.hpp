#pragma once

#include <cstdint>

#include <antwika/animation/Progress.hpp>

#include "antwika/tween/Easing.hpp"

namespace antwika::tween
{

    using antwika::animation::Progress;

    [[nodiscard]] std::int64_t tweenBetween(
        std::int64_t from,
        std::int64_t to,
        Easing easing,
        Progress progress);

}
