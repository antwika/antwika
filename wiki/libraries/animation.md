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
| `Facing.hpp` | `Facing` | The four directions, and `facingIndex()`; speculative, see below. |
| `DirectionalClipSet.hpp` | `DirectionalClipSet` | One clip per facing, sharing a single elapsed count; speculative, see below. |
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

The rule it is serving is the one the whole project turns on: only externally-supplied input is persisted, and everything the engine can regenerate deterministically is regenerated instead of recorded.
Which frame a walker's leg is on at tick 4,096 is a consequence of the tick number and the clip and nothing else, so it has no business being in a recording — and the way to keep it out of one is to make it impossible to put there.
A library that cannot hold a frame number between calls can never be asked to carry one.

This is the same shape as [`ui`](ui.md), which builds a frame of plain values and touches a renderer only in `paint()`, and for the same reason: a picture that is a value can be asserted with `EXPECT_EQ`, recomputed from scratch, and cannot be half-updated.

**The caller counts.**
An app that wants a clip to start when a walker was placed keeps that walker's start tick in its own state — which the replay already reproduces, since the placement came from a recorded click — and passes `now - start`.
An app that wants every walker in step passes the engine's tick straight through.
Both are one subtraction, and both are the caller's decision about what the animation is anchored to, which is a question about the app's state rather than about animation.

`DirectionalClipSet` falls out of the same choice.
A walker that turns a corner keeps its elapsed count and asks a different clip, so the walk cycle carries on through the turn instead of restarting.
Four little players, one per facing, would each be at a different point and would need resynchronising by hand; four clips and one number cannot get out of step, because there is only one number.

**`DirectionalClipSet` and `Facing` are speculative, and nothing in this tree uses them.**
[`game`](../apps/game.md) is the one application with anything that turns a corner, and it has a facing of its own — `game::Direction`, in `Direction.hpp`, which `Walking.hpp`'s `nextFacing()` yields — because what it needs is the direction a path step went on an isometric grid rather than one an animation library chose to name.
So the pair is kept for the argument above and adopted by nobody, and saying so is what [`tween`](tween.md) does with its own non-users: a page that reads as a description of live code when it is not is the kind of documentation that costs a reader time.
`resolve(Clip, Tick)` is the overload every caller actually uses, and [`companion`](../apps/companion.md) is the only one, through `PetScene`'s breathe, blink and drowse clips.

**`Progress` is a numerator and a denominator, not a `float`.**
Integer division agrees across GNU, LLVM and MinGW alike; a rounded fraction does not, and an assertion on a drawn position would become an assertion with a tolerance.
The moment a fraction has been rounded once, nothing downstream can tell how far it drifted or in which direction.
`interpolate()` divides exactly once, last, in the caller's own units, so the one place a half-pixel is lost is a place a reader can judge.

The units stay the caller's throughout: `interpolate()` takes two `std::int64_t`s and has no idea whether they are pixels, cells or anything else, since this library may not depend on [`gfx`](gfx.md) and so cannot name a point, a rectangle or a texture.
Turning a `Frame` index into an atlas slot is the application's job, because the application is the only thing that knows the picture.

Equality on a `Progress` is on the pair, so a test asserts exact numerators and never normalises.

**`AnimationError` is only ever thrown while a value is being built.**
A clip with no frames, a frame lasting zero ticks, a `Progress` larger than one: all are contradictions caught the moment the value is constructed.
That is what lets `resolve()` have no error path at all.

## See also

- [game](../apps/game.md) — `WalkerMotion.hpp` interpolates a walker's slide through `interpolate()`.
- [`blog/012-a-window-that-cant-talk-back.md`](../../blog/012-a-window-that-cant-talk-back.md) — why a renderer may never feed back into the loop.
