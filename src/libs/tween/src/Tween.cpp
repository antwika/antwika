#include "antwika/tween/Tween.hpp"

#include <cstdint>

#include <antwika/animation/Progress.hpp>

#include "antwika/tween/Ease.hpp"
#include "antwika/tween/Easing.hpp"

namespace antwika::tween
{

    std::int64_t tweenBetween(
        std::int64_t from,
        std::int64_t to,
        Easing easing,
        Progress progress)
    {
        return antwika::animation::interpolate(
            from, to, ease(easing, progress));
    }

} // namespace antwika::tween
