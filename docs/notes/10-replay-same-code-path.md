# 10 — Replay playback uses the same code path as a live run

**Status:** done. `IReplaySource`/`ReplaySource` (feeds back a previously
recorded/scripted `vector<TimedEvent>`, one tick's worth at a time) and
`EngineLoop` (the orchestrator: each tick, `dispatcher.setTick(tick)`, pull
and dispatch that tick's source events, then `engine.step(tick)`) landed in
the `replay` lib.

## Issues encountered

None so far. One deliberate simplification versus a first reading of
PLAN.md §3.7: `EngineLoop` takes the concrete `TickedEventDispatcher` type
directly rather than a further interface over it, since there is currently
only one implementation and it already sits behind `IEventDispatcher` for
its own dispatch behavior — adding another interface purely to make
`setTick` mockable wasn't justified by an actual second implementation or a
test that needed it (`EngineLoopTest` uses a real `TickedEventDispatcher`
wrapping a `MockEventDispatcher`, which was sufficient).

## Rationale/motivation

This is the crux of the determinism guarantee. If "replay mode" were a
separate code path from "live mode" (e.g. a bespoke function that just
re-applies events to state directly), then "replay reproduces the same
state" would only be true to the extent the two implementations happen to
agree — a fact that erodes the moment either one changes. Making replay mode
differ from live mode in **only** where events for a tick come from means
the tick/dispatch machinery itself is the single source of truth for how
state evolves, in both modes. See
[PLAN.md §3.7](../PLAN.md#37-feeding-a-loaded-replay-back-into-the-engine).

## How it's satisfied

`IReplaySource`/`ReplaySource` expose `eventsFor(tick) -> vector<Event>`. The
tick-loop orchestrator, each tick, asks either live input (real run) or an
`IReplaySource` (replay run) for that tick's events, dispatches them through
the identical `TickedEventDispatcher`, then calls `engine.step(tick)` either
way.

## Issues encountered

_(filled in during implementation)_
