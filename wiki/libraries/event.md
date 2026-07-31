# antwika::event

`src/libs/event/` — the one event mechanism.

## What it is for

Carrying named data through the system, and letting anything observe it.
There is exactly one mechanism, with no special-casing between events the engine defines and events an application defines.

## Key types

| Header | Type | Role |
| --- | --- | --- |
| `Event.hpp` | `Event` | `{ std::string name; std::string payload; }`, comparable with `operator==`. |
| `TickEvent.hpp` | `TickEvent` | An `Event` bound to the `time::Tick` it occurred on. |
| `IEventDispatcher.hpp` | `IEventDispatcher` | Registers `IEventSink`s and dispatches an `Event` to them. |
| `IEventSink.hpp` | `IEventSink` | `handle(const Event &)`. |
| `ITickEventSink.hpp` | `ITickEventSink` | `handle(const TickEvent &)` — the tick-aware form, and what apps mostly implement. |
| `EventDispatcher.hpp` | `EventDispatcher` | The plain implementation. |
| `TickedEventDispatcher.hpp` | `TickedEventDispatcher` | Stamps the current tick onto each event and fans it out to `ITickEventSink`s. |
| `EventRecorder.hpp` / `TickEventRecorder.hpp` | `EventRecorder`, `TickEventRecorder` | Collect what passed through, for a `--record` run. |

`MockEventDispatcher`, `MockEventSink` and `MockTickEventSink` live under `tests/mocks/`.

## Depends on

[`time`](time.md) only.

## Non-obvious decisions

**An event's payload is an opaque string.**
The library does not know or care whether it holds JSON, a comma-separated list or nothing.
That is what lets an application define `task.submit` with a `id,priority,durationTicks,label` payload and `input.pointer_down` with a JSON object, through the same type, with no variant, no registry and no template parameter running through the whole system.

**Names are dot-namespaced by the module that owns them.**
`engine.tick`, `input.pointer_move`, `game.score_increment`, `life.toggle_cell`, `task.submit`, `poker.buy_in`.
Names in `input.*` are part of the replay file format and may not be changed once a replay has been written with one.

**`TickEventRecorder` is the only place a recording may be thinned.**
Reducing what reaches the recorder is done by decorators *upstream* of it, on the source side, so the file always holds exactly what the run consumed.
Filtering after the recorder would make the file disagree with the run that wrote it.
