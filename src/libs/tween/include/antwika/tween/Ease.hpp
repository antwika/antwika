#pragma once

#include <antwika/animation/Progress.hpp>

#include "antwika/tween/Easing.hpp"

namespace antwika::tween
{

    using antwika::animation::Progress;

    /**
     * @brief Shape a fraction by an easing curve.
     *
     * A pure function of its two arguments, holding nothing between
     * calls, for exactly the reason antwika::animation has no `Animator`
     * you advance: a tween that remembered where it had got to would be
     * simulation state hiding in whatever drew it.
     *
     * The result is exact.
     * A curve of power `k` over a fraction `n/d` comes back over `d` to
     * the `k`, and the fraction is **not** reduced -- `Progress` compares
     * on the pair rather than the value, so reducing would change what a
     * caller's own equality assertions mean.
     * The practical consequence is that the denominator grows quickly:
     * a quintic over `1/8` comes back over `32768`, which is nothing, and
     * a quintic over a denominator in the millions does not fit, which is
     * what TweenError reports.
     *
     * Every curve is anchored: `ease(anything, 0/d)` is zero and
     * `ease(anything, d/d)` is one, so a span's two ends are exactly its
     * two ends whatever shape the middle takes.
     *
     * @param easing Which curve to shape by.
     * @param progress How far along the span is, unshaped.
     * @return How far along it is once the curve has been applied.
     * @throws TweenError If the arithmetic would leave a
     * antwika::time::Tick -- see the denominator note above.
     */
    [[nodiscard]] Progress ease(Easing easing, Progress progress);

} // namespace antwika::tween
