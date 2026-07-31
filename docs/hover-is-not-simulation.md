# Hover is not part of the simulation

Hover, and pointer-driven appearance generally, is visual candy.
It may decide what a frame looks like and it may decide nothing else.
This page states that rule, says which values it governs, and explains why it is enforced by the shape of the types rather than by a convention somebody has to remember.

It is the `antwika::ui` reading of the rule [`input::PointerHintChannel`](../src/libs/input/include/antwika/input/PointerHintChannel.hpp) already states for a free-moving pointer, and the two are the same rule seen from either end.

## The rule

**What may be read off a hover channel: what is drawn.**
A widget's fill colour, a highlight, a custom cursor, a placement ghost -- anything that exists only in the picture and is recomputed from scratch every frame.

**What may never be: anything a replay reproduces.**
Which widget a press activated, where focus went, what a text field became, which option was chosen, a score, a grid cell, a component staged into an `ecs::World`, a save file, a value a `--record` file holds or a value derived from one.

The reason is one sentence, and it has nothing to do with taste.
A live run and its replay do not agree about where the pointer is between two clicks, deliberately: the movements in that gap are thinned out of the recorded stream by `input::IdleMotionSource`, and a replay therefore publishes only the positions its recorded events happen to carry.
Fold a hint into anything a replay reproduces and the run and its replay diverge, silently, with the symptom nowhere near the line that caused it.

## Why the recording is gated in the first place

A window system reports pointer motion at its own rate rather than the application's.
SDL will report several hundred movements a second into a run that ticks twenty-five times, and between two clicks none of them decides anything.
`input::IdleMotionSource` holds that motion back so a `--record` file does not grow at the window system's rate, and its one documented cost used to be that a button could light up only on the press -- there was simply nothing in the tick stream to draw a hover from.

`input::PointerHintChannel` gives the drawing back without giving the recording back with it.
It is a value cell, written once per tick by `input::PointerHintSource` and read through an accessor named `forRenderingOnly()`.
So an application that wants both attaches both, and the two settings are a pair rather than a choice.

## The two halves, and where the line between them is

`antwika::ui` resolves a frame in two places now, and they are deliberately different kinds of thing.

**Inside the tick path, from the recorded stream**: `ui::Context` takes a `ui::Pointer`, a `ui::Keyboard` and an incoming focus, and `finish()` produces `ui::Interactions`.
Every field of that value -- `activated`, `focused`, `edit`, `chosen`, `hovered`, `pointerOverUi` -- is a function of recorded input and of the layout, both of which a replay reproduces exactly.
None of this changed, and none of it may.

**On the render side, from the hint**: `ui::applyHover()` takes a `ui::DrawList`, a `ui::HoverTargets` and a `ui::HoverPointer`, and rewrites the colour of the fills the targets name.
It is handed no `ui::Frame`, no `ui::Interactions` and no arena, so there is no field it could write an answer into.

The line between the two is the argument list.

## Why this is structural rather than a promise

Three separate decisions carry the guarantee, and each removes a way of getting it wrong.

**`ui::HoverPointer` carries a position and nothing else.**
There is no `down` and no `pressed`, unlike `ui::Pointer`.
A hover pointer cannot say that a button went down, so it cannot activate anything -- not because a rule forbids it but because the value has no field that could mean it.

**`applyHover()` is given the picture and only the picture.**
A `ui::Frame` holds four things, and the function sees one of them, by reference, plus two read-only values.
`Interactions` is not reachable from inside it at any cost, so "the hover decided what was activated" is not a mistake that can be made here, reviewed for, or regressed into.

**The hint is a value cell rather than a marked event.**
The cheaper shape would be a second event kind carrying a "do not record" marker that `TickEventRecorder` honours by skipping it.
That was rejected where the channel was introduced, and the reason applies here unchanged: a marked event still travels the dispatcher, so every `ITickEventSink` an application owns is handed it, and the condition stops being a property of the wiring and becomes a rule each sink has to remember.
A value cell reaches a sink only if somebody passed that sink the channel in a `main.cpp`, in the open, next to the pipeline that publishes it.

## What `applyHover()` actually does, and why

It decides the appearance of **every** target rather than only the one under the pointer.
The frontmost target the position falls inside is painted hovered; every other is painted idle.

Lighting one up without putting the others out would be wrong in a way that only shows up in a gated run.
The recorded stream carries a position where a press needed one, so `Interactions::hovered` names whichever widget the last press passed over -- and that widget would stay lit for the rest of the session.
Putting them all out is what makes hover appearance entirely the hint's business once an application opts in, which is the honest reading of "hover is not part of the simulation".

A **held** target is stepped over either way.
A press is recorded input, resolved inside the tick path like every other interaction, so a button being pressed goes on looking pressed rather than being repainted as merely hovered.
`ui::HoverTarget::held` is what says so, and it is written by `resolve()` from the recorded pointer, never from the hint.

Called with a hover pointer reporting no position, it changes nothing at all.
So an application that never opts in draws byte for byte the picture `finish()` produced, and every existing caller is unaffected.

## The seam between the libraries

`antwika::ui` depends on `antwika::gfx` and nothing else, and `antwika::input` knows nothing about a UI.
Neither may name the other, so `antwika::app` is where an application says the two describe the same pointer:

- `app::pointerFrom(state, located)` folds recorded edges into a `ui::Pointer`.
- `app::hoverFrom(channel.forRenderingOnly())` reads a hint as a `ui::HoverPointer`.

Two functions rather than one with an extra field, because the two positions come from different places and mean different things, and keeping them apart in the type system is what stops one being passed where the other belongs.

## The worked example

`apps/gfx_demo` runs both halves.
It has no recorder of its own, so `DemoLoop` gates the position out of its `ui::Pointer` by hand -- modelling exactly what `IdleMotionSource` would leave in a recording -- and publishes the free-moving position to a `PointerHintChannel` it owns.
The UI's own hit-test therefore knows where the pointer is only while a button is held, and the buttons light up on approach all the same.

`DemoLoopTest` asserts both ends of that: a movement with no press draws the hovered fill, and it activates nothing.

## What is asserted, and where

- `src/libs/ui/tests/HoverTest.cpp` -- `applyHover()` alone: the frontmost target wins, every other goes idle, a held one is untouched, a mismatched target changes nothing, and no position changes nothing.
- `src/libs/ui/tests/ContextHoverTest.cpp` -- a real frame's targets, and the load-bearing one: a frame with an activation, a focus, an edit and a chosen option in play has identical `Interactions` before and after a hover pass, at every position tried.
- `src/libs/app/tests/HoverRecordingTest.cpp` -- the end-to-end claim: the same session driven through `InputPipeline` twice, once with a hint channel and a hover pass and once with neither, records exactly the same events on exactly the same ticks, while only the second one's pictures follow the pointer.

## Adopting it

An application already drawing a `antwika::ui` frame adopts this in three lines, and the order is the whole of the discipline:

1. Describe and resolve the UI inside the tick path from the recorded `ui::Pointer`, exactly as now.
2. Act on `Frame::interactions`, and re-describe if the state changed.
3. Only then, on the render side, call `applyHover(frame.commands, frame.hoverTargets, hoverFrom(hints.forRenderingOnly()))`.

Nothing between steps one and two may read the channel.
If a sink needs the hint to decide something, that something is not a hover, and it belongs in the recorded stream instead.
