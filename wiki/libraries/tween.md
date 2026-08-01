# antwika::tween

`src/libs/tween/` — shaping a fraction by a curve, exactly.

## What it is for

Turning "a third of the way along" into "a third of the way along, but easing out", without leaving exact arithmetic.

[`animation`](animation.md) already answers *how far through a span we are* and interpolates linearly between two ends.
This library is the piece between those two: it takes a `Progress` and gives back a `Progress`, shaped by a curve.
A caller that wants the value as well as the fraction uses `tweenBetween()`, which is exactly `interpolate(from, to, ease(easing, progress))` and exists so that composition does not have to be rediscovered at every call site.

## Key types

| Header | Type | Role |
| --- | --- | --- |
| `Easing.hpp` | `Easing`, `easingIndex()`, `easingName()` | Which curve, and its index and name. |
| `Ease.hpp` | `ease()` | Shape a `Progress` by a curve. |
| `Tween.hpp` | `tweenBetween()` | Shape a fraction and interpolate with it, in one call. |
| `TweenError.hpp` | `TweenError` | Thrown when the arithmetic would not fit. |

## Depends on

[`animation`](animation.md), for `Progress` and `interpolate()`, and nothing else.

It therefore cannot name a pixel, a point or a rectangle, exactly as `animation` cannot: the units are always the caller's own.

## Non-obvious decisions

**The curve list is short, and that is the whole design.**
Sixteen easings: linear, then quad, cubic, quart and quint in `In`/`Out`/`InOut`, then bounce in the same three.
Every one is a polynomial or a piecewise polynomial, so it can be computed with integer arithmetic on a rational and comes out identical on GNU, LLVM and MinGW alike.

**Sine, exponential, circular and elastic are deliberately absent.**
Each needs a transcendental or a square root, and none of those is bit-identical across standard libraries.
That is the same argument [`rng`](rng.md) makes about `<random>`'s distributions, arrived at from the other side: a curve that disagreed in its last bit would put a drawn position a pixel out on one toolchain and not another, and every assertion on a drawn position in this project would have to become an assertion with a tolerance.

**Back and anticipate are absent for a second, separate reason.**
They overshoot — back-in dips below zero before it climbs, back-out passes one before it settles.
A `Progress` is between zero and one inclusive *by construction*, so an overshoot is not merely inexact here, it is unrepresentable.
Admitting one means widening `Progress`, which is a change to `animation` and a decision of its own rather than something this library should force.

**A bounce is four parabolas, which is why it is here at all.**
It looks like it needs something exotic and does not: `121/16 (t - a)² + c` in four pieces, where every offset is over an eleventh or a twenty-second.
The 121 cancels against the square of that denominator, so the whole curve stays in whole numbers.
Every piece is put over `64 d²` rather than each over its own, so which piece answered is not something a caller can read off the denominator it gets back.

**The fraction is not reduced.**
`Progress` compares on the pair rather than the value — `1/2` and `2/4` are different values there, because a clip that ran a frame for four ticks did not run it for two — so reducing would quietly change what a caller's own equality assertions mean.
The consequence is that the denominator grows with the curve's power: a quintic over `1/8` comes back over `32768`, which is nothing, and a quintic over a denominator in the millions does not fit.

**Not fitting is refused rather than wrapped.**
`TweenError` is thrown when the arithmetic would leave a `time::Tick`.
That is [`pathfinding`](pathfinding.md)'s call about an overflowing cost, for the same reason: a wrapped fraction is a position quietly in the wrong place, where a refusal tells the caller its denominator is too large for the curve it asked for.

**A curve is a table row, not a switch arm.**
`ease()` indexes a table by `easingIndex()`, so a new easing is a row rather than an arm somebody can forget, and there is no out-of-range default that no input could reach.
`Easing` is a `std::uint8_t`, so a caller *can* cast a number no enumerator has into one; that is refused rather than indexed with.

**There is nothing to advance.**
`ease()` is a pure function of its two arguments and holds nothing between calls, for exactly the reason `animation` has no `Animator`: a tween that remembered where it had got to would be simulation state hiding in whatever drew it, and it would look right on a live run and drift on a replay.

## Where it is worth using

Nowhere yet — nothing in the tree links it.

The natural fits are the render-side motions that currently move linearly or not at all: a camera pan settling, a UI panel sliding, a building appearing, a companion's idle motion.

**`apps/game`'s walker motion is not one of them**, despite being the case that prompted the library.
A walker crosses many cells in a row, and easing each cell's step would make it start and stop at every tile — a walk cycle that lurches rather than one that walks.
Easing the *camera* that follows it is the version of that idea which reads correctly.
