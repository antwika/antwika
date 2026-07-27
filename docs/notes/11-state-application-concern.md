# 11 — State is an application concern

**Status:** done. `GameState` (plain data) + `GameStateReducer`
(`ITimedEventSink`) landed in `apps/game` — no state concept was added to
`engine`, `event`, or `replay`.

## Rationale/motivation

The engine core (`engine`, `event`, `replay` libs) has no idea what "score"
or "player" means, and shouldn't — baking domain state into the engine would
violate SRP (the engine would own both "how simulation progresses" and "what
a specific game's state looks like") and make the engine core useless for
any other game built on it. State is therefore designed and demonstrated at
the `apps/game` layer. See
[PLAN.md §3.8](../PLAN.md#38-state-suggested-representation-kept-out-of-the-engine-core)
for the full comparison of alternatives considered.

## How it's satisfied

Plain-data state (a struct with `operator==`, same pattern as `Event`) plus
one or more reducers implementing `ITimedEventSink` — the exact same
extension point [item 08](08-recording.md)'s `ReplayRecorder` uses. A
reducer folds a `TimedEvent` into its state in place; recording and updating
state are two implementations of the same interface, not two separate
systems. Live incremental reduction now; snapshotting is a possible future
optimization on top of the same reducer, not a redesign.

## Issues encountered

_(filled in during implementation)_
