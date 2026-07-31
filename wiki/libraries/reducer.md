# antwika::reducer

`src/libs/reducer/` — folding events into a plain state value.

## What it is for

The lightweight alternative to [`ecs`](ecs.md) for application state: a plain copyable struct, and a pure function that folds one event into it.

## Key types

| Header | Type | Role |
| --- | --- | --- |
| `IReducer.hpp` | `IReducer<State>` | `State reduce(const State &previous, const TickEvent &event) const` — pure, `[[nodiscard]]`, returning a new value. |
| `ReducerSink.hpp` | `ReducerSink<State>` | An `ITickEventSink` holding a `State &` and an `IReducer<State> &`, so any reducer plugs straight into the dispatcher. |

`MockReducer` lives under `tests/mocks/`.

## Depends on

[`event`](event.md) only.
It is an interface-only (`INTERFACE`) CMake target — both headers are templates, so there is nothing to compile.

## Non-obvious decisions

**`reduce()` returns a new state rather than mutating one.**
It is the same discipline `ecs` applies to systems, applied to plain-struct state: no side effects, so folding the same events over the same starting state always lands in the same place.
`State` must therefore be copyable.

**The sink is the only mutating thing, and it is two lines.**
`ReducerSink` is the whole adapter between "a pure fold" and "an engine that dispatches events": every dispatched event becomes one `state = reducer.reduce(state, event)`.
Keeping the mutation in one tiny, tested class is what lets the reducers themselves stay pure functions that need no engine to test.

[`apps/game`](../apps/game.md)'s `GameState` and `GameStateReducer` are the worked example.
