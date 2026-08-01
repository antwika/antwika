#pragma once

#include <stdexcept>

namespace antwika::tween
{

    /**
     * @brief Thrown when an easing could not be computed exactly.
     *
     * There is exactly one cause: the arithmetic would leave the range of
     * a antwika::time::Tick.
     * Shaping a fraction raises its denominator to the curve's power, so
     * a quintic over a denominator of `d` works in `d` to the fifth, and
     * a caller handing in a large enough denominator runs out of room.
     *
     * Refusing rather than saturating or wrapping is the same call
     * antwika::pathfinding makes about a cost that overflows: a wrapped
     * fraction is a position quietly in the wrong place, where a refusal
     * is a caller being told its denominator is too big for the curve it
     * asked for.
     *
     * Deliberately a single, specific, catchable type, mirroring
     * antwika::animation::AnimationError, which this library sits beside.
     */
    class TweenError final : public std::runtime_error
    {
    public:
        using std::runtime_error::runtime_error;
    };

} // namespace antwika::tween
