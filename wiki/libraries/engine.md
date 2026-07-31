# antwika::engine

`src/libs/engine/` — the fixed-timestep core.

## What it is for

Advancing a simulation one discrete tick at a time, and nothing else.
The engine is domain-agnostic: it knows about ticks and about dispatching events, and has no idea what any application's state looks like.

## Key types

| Header | Type | Role |
| --- | --- | --- |
| `IEngine.hpp` | `IEngine` | Two methods: `start()` and `step(time::Tick)`. |
| `Engine.hpp` | `Engine` | The implementation, holding an `event::IEventDispatcher` and an `log::ILogger`. |
| `Events.hpp` | `engine::events::kTick`, `kStop` | The two built-in event names, `"engine.tick"` and `"engine.stop"`. |
| `StopSignal.hpp` | `StopSignal` | An `ITickEventSink` that latches when it sees `engine.stop`; `EngineLoop` consults it after each tick. |

`MockEngine` is available under `tests/mocks/` for testing code that drives an engine.

## Depends on

[`event`](event.md), [`log`](log.md), [`time`](time.md).

## Non-obvious decisions

**`engine.tick` is dispatched by the engine, so it is never recorded.**
`Engine::step()` emits it at the start of every tick, before that tick's queued events are processed.
Because the engine regenerates it deterministically, it is exactly the kind of event a replay must *not* persist — a replay that stored ticks would produce two per tick when replayed.

**`engine.stop` is genuine external input and therefore is recorded.**
The asymmetry between the two built-in names is deliberate and is the clearest illustration of the recording rule: `kTick` is regenerated, `kStop` is not, so a replay must carry `kStop` to stop at the same tick a live run did.
`StopSignal` exists so that stopping is an observation of the event stream rather than a side channel out of the loop.

**No RNG.**
There is no random number generator in the engine and no field reserved for one later; anything that needs randomness (such as [`holdem`](holdem.md)) seeds its own deterministic generator from recorded configuration.
