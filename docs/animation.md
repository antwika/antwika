# The animation library holds no time of its own

`antwika::animation` is a library for saying which picture something is showing, and it contains no clock, no timer, no elapsed counter and no `advance()`.
A `Clip` is a definition -- an ordered run of `KeyFrame`s and a `LoopMode` -- and `resolve(clip, elapsedTicks)` is a pure function of that definition and a number the caller passes in.
Nothing in the library is mutable after construction, and calling it twice with the same arguments always gives the same `Frame`.

This document is about why that is the design rather than an omission, because the obvious alternative -- an `Animator` object you tick -- is what almost every other engine ships, and it is exactly the thing this project cannot have.

## The rule it is serving

`antwika::replay::EngineLoop` is the one code path shared by a live run and a replay, and what makes a replay reproduce a session is that only externally-supplied input is persisted.
Everything the engine can regenerate deterministically is regenerated instead of recorded, which is why `engine.tick` is never written to a replay file and why `apps/game` stores a click rather than the tile the click placed.

An animation is regenerable in exactly that sense.
Which frame a walker's leg is on at tick 4,096 is a consequence of the tick number and the clip, and nothing else, so it has no business being in a recording.
The way to keep it out of a recording is to make it impossible to put there: if the library cannot hold a frame number between calls, no recording can ever be asked to carry one.

## What an animator object would cost

Suppose `Animator` held `elapsed` and offered `advance()` and `currentFrame()`.
It would then be simulation state, and it would have to follow every rule simulation state follows in this project: it would have to be stepped inside the tick path, exactly once per tick, downstream of the recorder, and never from a renderer.
A renderer that called `advance()` would be feeding back into the loop, which is the one thing `blog/012-a-window-that-cant-talk-back.md` says a renderer must never do -- and it is a mistake nobody would notice, because on a live run the picture would look right.

The failure would only surface as a replay drifting a frame or two out over a long session, on a machine that dropped a frame somewhere.
That is the worst kind of bug this codebase can have: silent, timing-dependent, and invisible until the artefact it corrupts is the one thing meant to be trustworthy.

Making `resolve()` a free function of `(Clip, Tick)` deletes the whole category.
There is nothing to step, so there is no wrong place to step it from, and a renderer asking what to draw is a read like any other.
This is the same shape as `antwika::ui`, which builds a `ui::Frame` of plain values and touches a renderer only in `paint()` -- and for the same reason: a picture that is a value can be asserted with `EXPECT_EQ`, recomputed from scratch, and cannot be half-updated.

## Where the tick comes from

The caller counts.
An app that wants a walker's clip to start when the walker was placed keeps that walker's start tick in its own state -- which the replay already reproduces, since the placement came from a recorded click -- and passes `now - start`.
An app that wants every walker in step passes the engine's tick straight through.
Both are one subtraction, and both are the caller's decision about what the animation is anchored to, which is a question about the app's state rather than about animation.

`DirectionalClipSet` falls out of the same choice.
A walker that turns a corner keeps its elapsed count and asks a different clip, so the walk cycle carries on through the turn instead of restarting.
Four little players, one per facing, would each be at a different point and would need resynchronising by hand; four clips and one number cannot get out of step because there is only one number.

## Why the progress value is a fraction

`Progress` is a numerator and a denominator, not a `float`.

The reason is the same determinism argument one level down.
A replay only reproduces a picture if every arithmetic step agrees across builds, and integer division agrees everywhere this project compiles, across GNU, LLVM and MinGW alike.
Floating point is close to that and not quite it: the moment a fraction has been rounded once, nothing downstream can tell how far it drifted or in which direction, and an assertion on a drawn position becomes an assertion with a tolerance.

Keeping the two numbers apart also puts the rounding where somebody can see it.
`interpolate(from, to, progress)` divides exactly once, last, in the caller's own units, so a half-pixel is lost at the one point where a reader can judge whether losing it matters.
An intermediate `0.5f` scattered through a chain of multiplications gives no such point.

The units are the caller's throughout.
`interpolate()` takes two `std::int64_t`s and has no idea whether they are pixels, cells or anything else, because this library may not depend on `antwika::gfx` and therefore cannot name a point, a rectangle or a texture.
A `Frame` is an index and a fraction; turning that index into an atlas slot is `apps/game`'s job, and `apps/game` is the only thing that knows the picture.

## What is left to reject

One exception type, `AnimationError`, and it is only ever thrown from a constructor or a factory.
A clip with no frames, a frame lasting zero ticks, a total duration that would not fit in a `Tick`, a `Progress` larger than one, a step of zero ticks: all of them are contradictions in the numbers, and all of them are caught the moment the value is built.

That is what lets `resolve()` have nothing to check.
A `Clip` that exists is a clip that can be resolved at every tick from zero upward, so the resolving code has no error path, no failure to report and no state in which it could be asked something it cannot answer.
