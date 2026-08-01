#pragma once

#include <cstdint>

#include <antwika/animation/Progress.hpp>

#include "antwika/tween/Easing.hpp"

namespace antwika::tween
{

    using antwika::animation::Progress;

    /**
     * @brief Get a whole-number value an eased fraction of the way
     * between two others.
     *
     * Exactly `animation::interpolate(from, to, ease(easing, progress))`,
     * and it exists because that composition is what this library is for:
     * a caller wanting a value tweened between two others should not have
     * to know that the shaping and the interpolating live in two places.
     *
     * The units are the caller's own -- pixels, cells, whatever the two
     * ends were expressed in -- since neither this library nor
     * antwika::animation has any idea what is being moved.
     * The division truncates towards zero and happens exactly once, last,
     * so the answer is the same on every toolchain rather than the same
     * to within a rounding mode.
     *
     * @param from Where the span starts.
     * @param to Where the span ends.
     * @param easing Which curve to shape the fraction by.
     * @param progress How far along the span to look, unshaped.
     * @return The value that far along, exactly `from` at 0/n and exactly
     * `to` at n/n whatever the easing.
     * @throws TweenError If the easing's arithmetic would leave a
     * antwika::time::Tick -- see ease().
     * @note The span times the eased numerator must fit in a signed
     * 64-bit integer, which every offset a screen can hold does.
     */
    [[nodiscard]] std::int64_t tweenBetween(
        std::int64_t from,
        std::int64_t to,
        Easing easing,
        Progress progress);

} // namespace antwika::tween
