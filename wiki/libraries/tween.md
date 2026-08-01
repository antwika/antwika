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

## Who uses it

Two applications, and the honest answer is that they are the only two with anything to tween.

[`apps/companion`](../apps/companion.md) breathes on `Easing::QuadInOut`, through `kBreatheEasing` in `PetScene.cpp`.
The breath was four poses a whole grid unit apart, stepped — eight pixels of jolt, twice a breath.
It is now tweened across the two rows each frame sits between, which is the case this library was worth building for: a linear breath reads mechanical, and a sine one is what a breath actually wants and is not exact, so `QuadInOut` is the closest curve that stays in whole numbers.

[`apps/game`](../apps/game.md) draws a walker's slide through `game::kWalkerEasing` in `WalkerMotion.hpp`, set to `Easing::Linear`.

**Linear is the whole point of that call site rather than a placeholder in it.**
A walker crosses many cells in a row, so easing each cell's step would make it start and stop at every tile — a walk cycle that lurches rather than one that walks.
What the tween buys there is not a curve but a *named place for the decision*: the easing is one constant, documented, in a header, instead of a linear interpolation nobody would think to question.

Linear is also the one curve that provably cannot refuse.
Every other easing raises the denominator to its curve's power and can therefore run out of room; linear raises it to the first, so `walkerBounds()` calls it from a renderer without a guard and without a `try`.
Any other easing there would need one, which is a second reason that call site is not the place to experiment.

Easing the camera that follows a walker is the version of "ease the walker" that reads correctly.

## Who does not, and why

The other ten applications have nothing to tween, and that is a fact about them rather than a gap.

A tween needs two things: a value that moves, and more than one picture drawn while it is moving.
Only `game` draws more than one frame per tick, through `app::FramePacedSource`; and only `companion` runs a multi-tick animation it can resolve a position out of.

`life` toggles cells, `atlas_editor` paints pixels and `sudoku` writes a digit into a square — all discrete, with nothing in between two states to be part-way through.
`poker` redraws a static table each tick, and `tower_defence` advances a mob one whole cell per tick and draws it once, so tweening it would mean giving that app sub-tick frames first — a change to how it runs, not a change to how it draws.
`gfx_demo`, `gfx3d_demo`, `sound_demo` and `ui_demo` are showcases for other libraries; `gfx3d_demo`'s cube turns at a constant rate, which is the one motion that should *not* ease.

Adding a dependency to any of them would be adoption for its own sake.
