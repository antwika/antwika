# antwika::animation

`src/libs/animation/` — which picture something is showing, resolved as a pure function.

## What it is for

Saying which frame of an animation to draw at a given tick.
It holds no clock, no timer, no elapsed counter and no `advance()`, and nothing in it is mutable after construction.

A `Clip` is a definition — keyframes plus a loop policy — and `resolve(clip, elapsedTicks)` is a pure function of that definition and a number the caller passes in.

## Key types

| Header | Type | Role |
| --- | --- | --- |
| `KeyFrame.hpp` | `KeyFrame` | One frame index and how many ticks it lasts. |
| `Clip.hpp` | `Clip` | An ordered run of keyframes plus a `LoopMode`; `uniformClip()` builds an even one. |
| `LoopMode.hpp` | `LoopMode` | Whether a clip repeats or holds its last frame. |
| `Frame.hpp` | `Frame`, `resolve()` | The answer: a frame index and how far into it the tick is. |
| `Progress.hpp` | `Progress`, `interpolate()` | An exact rational position, and interpolation over `std::int64_t`. |
| `Facing.hpp` | `Facing` | The four directions, and `facingIndex()`. |
| `DirectionalClipSet.hpp` | `DirectionalClipSet` | One clip per facing, sharing a single elapsed count. |
| `Playback.hpp` | `stepProgress()` | Folding a whole-tick phase and a sub-tick fraction into one rational. |
| `AnimationError.hpp` | `AnimationError` | Thrown from constructors and factories only. |

## Depends on

[`time`](time.md), and nothing else.

It therefore cannot name a texture, a point or a rectangle: a `Frame` is an index the application maps to an atlas slot.

## Non-obvious decisions

**There is deliberately no `Animator` you tick.**
An object holding `elapsed` and offering `advance()` would be simulation state, so it would have to be stepped inside the tick path, exactly once per tick, and never from a renderer.
A renderer that called `advance()` would be feeding back into the loop — and it is a mistake nobody would notice, because on a live run the picture would look right.
The failure would surface only as a replay drifting a frame or two out over a long session.

Making `resolve()` a free function of `(Clip, Tick)` deletes the whole category: there is nothing to step, so there is no wrong place to step it from.

**The caller counts.**
An app that wants a clip to start when a walker was placed keeps that walker's start tick in its own state — which the replay already reproduces, since the placement came from a recorded click — and passes `now - start`.

**`Progress` is a numerator and a denominator, not a `float`.**
Integer division agrees across GNU, LLVM and MinGW alike; a rounded fraction does not, and an assertion on a drawn position would become an assertion with a tolerance.
`interpolate()` divides exactly once, last, in the caller's own units, so the one place a half-pixel is lost is a place a reader can judge.

Equality on a `Progress` is on the pair, so a test asserts exact numerators and never normalises.

**`AnimationError` is only ever thrown while a value is being built.**
A clip with no frames, a frame lasting zero ticks, a `Progress` larger than one: all are contradictions caught the moment the value is constructed.
That is what lets `resolve()` have no error path at all.

## See also

- [`docs/animation.md`](../../docs/animation.md) — the long-form argument.
- [game](../apps/game.md) — `WalkerMotion.hpp` interpolates a walker's slide through `interpolate()`.
