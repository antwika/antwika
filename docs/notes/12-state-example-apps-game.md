# 12 — State example in `apps/game`

**Status:** done. `GameStateReducer` is wired into `bootstrap()` as a timed
sink; `bootstrap()` now returns the resulting `GameState` so callers
(`main.cpp`, tests) can inspect it directly.

## Rationale/motivation

Requested explicitly: a small, concrete example of state representation and
usage in the actual game app — not just an abstract pattern in the engine
libs. It's also the piece that proves [items 06](06-extendable-events.md)
and [07](07-builtin-common-events.md) actually compose: state reacting to
both a built-in engine event and an app-defined custom event through the
identical mechanism.

## How it's satisfied

`GameState { ticksProcessed; score; }` (plain data) + `GameStateReducer`
(`ITimedEventSink`) in `apps/game`, registered as a timed sink in
`bootstrap()`. Reacts to `events::kTick` (built-in) by incrementing
`ticksProcessed`, and to a new custom `"game.score_increment"` event
(app-defined, payload-carrying) by adding to `score`. Deliberately small —
this is a demonstration, not a real game.

## Issues encountered

_(filled in during implementation)_
