# antwika::simulation

`src/libs/simulation/` — the tick loop, the seam it reads through, and the pacing around it.

## What it is for

Running a simulation.
This is where the project's central claim lives: a live run and a replayed one go through the same code path, differing only in what supplies each tick's events, so a replay reproduces state by construction.

Recording a run and reading one back is [`replay`](replay.md)'s, which depends on this library.
A live run therefore links no JSON, no schema validator and no migration chain.

## Key types

| Header | Type | Role |
| --- | --- | --- |
| `EngineLoop.hpp` | `EngineLoop` | Per tick: ask the source for events, dispatch them, step the engine, check `StopSignal`. |
| `ITickSource.hpp` | `ITickSource` | `eventsFor(tick)` — the single seam between live and replayed runs. |
| `EngineLoopError.hpp` | `EngineLoopError` | Loop misuse: `maxTicks` reached with no stop event. |
| `TickPacer.hpp` | `TickPacer` | An `ecs::ISystem` that sleeps through an injected `time::ISleeper` so a windowed run does not spin. |
| `WindowInputSource.hpp` | `WindowInputSource` | An `ITickSource` that turns a window close request into `engine.stop`. |

## Depends on

[`ecs`](ecs.md), [`engine`](engine.md), [`event`](event.md), [`gfx`](gfx.md), [`time`](time.md).
`ecs` is there for `TickPacer` and `gfx` for `WindowInputSource` — both narrow, deliberate edges.

## Non-obvious decisions

**The seam is named for what it does, not for one of its implementers.**
It was `IReplaySource` while a loaded file was the only thing that implemented it.
By the time `input::LiveInputSource`, `input::IdleMotionSource`, `input::PointerHintSource`, `app::FramePacedSource` and three applications' window sources implemented it too, the name said "recording" about things that never touch one.
`ITickSource` says what `eventsFor(tick)` does, and leaves the word replay to the library that has a file.

**Ticks are asked for once each, in increasing order.**
`EngineLoop` counts up from zero and asks once per tick, and implementations rely on it: `replay::ReplaySource` walks its recording with a cursor, `input::IdleMotionSource` carries a latched movement forward from the tick it arrived on.
Asking twice, or going back, is not a question any of them can answer.

**The tick carrying the stop still runs to completion.**
Its events are dispatched and the engine still steps before the loop exits, so a live run and its replay agree up to and including the terminal tick.

**Closing a window is input, not control flow.**
`WindowInputSource` decorates another source and appends `engine.stop` once the window has gone, so a close lands in a `--record` file like any other input and replays at the same tick.
It holds a `gfx::WindowId` rather than an `IWindow &`, so it cannot close anything — closing here would leave the tick that carries the stop drawing into a closed window.

**Pacing changes how long a run takes and never what it computes.**
`TickPacer` reads neither the `World` nor the tick it is given, and it is registered as the last observer, after whatever draws the frame, which makes the order present-then-wait.

**Drawing more often than the simulation ticks is a decorator, not a change to the loop.**
`EngineLoop::run()` is the one path live and replay runs share, and putting a render cadence in it would change its signature for every app to give one of them a feature.
[`app`](app.md)'s `FramePacedSource` wraps an `ITickSource` instead: it draws the extra frames in the gap before a tick's events are read, then returns what the inner source returned, unchanged.
It is a pure observer in exactly `PointerHintSource`'s sense.
