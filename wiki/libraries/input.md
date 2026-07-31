# antwika::input

`src/libs/input/` — keyboard and pointer, as replayable edges.

## What it is for

Reading a keyboard and a pointer through an abstraction, and delivering what it reads into the tick loop as ordinary replay input.
As with [`gfx`](gfx.md), the concrete frameworks live under `backends/` and no file under `src/` names one.

## Key types

| Header | Type | Role |
| --- | --- | --- |
| `IInputBackend.hpp` | `IInputBackend` | Polls a queue of `InputEvent`s and declares its `InputCapabilities`. |
| `InputEvent.hpp` | `InputEvent` | A `std::variant` of `KeyPressed`, `KeyReleased`, `PointerMoved`, `PointerButtonPressed`, `PointerButtonReleased`, `PointerScrolled`. |
| `Key.hpp`, `MouseButton.hpp`, `KeyModifiers.hpp` | `Key`, `MouseButton`, `KeyModifiers` | Symbolic names, with `toString()`. |
| `Position.hpp`, `Offset.hpp` | `Position`, `Offset` | Where and how far. |
| `Events.hpp` | `input::events::kKeyDown`, `kKeyUp`, `kPointerMove`, `kPointerDown`, `kPointerUp`, `kPointerScroll` | The event names, which are part of the replay file format. |
| `InputEventCodec.hpp`, `IInputEventCodec.hpp` | `InputEventCodec` | Encodes an `InputEvent` as an `event::Event` with a JSON payload, and back. |
| `LiveInputSource.hpp` | `LiveInputSource` | The `IReplaySource` that puts live edges into the tick stream. |
| `CoalescingPointerSource.hpp` | `CoalescingPointerSource` | Keeps only the last of each run of movements inside a tick. |
| `IdleMotionSource.hpp` | `IdleMotionSource` | Holds back movement arriving while no button is held. |
| `StopOnKeySource.hpp` | `StopOnKeySource` | Appends `engine.stop` when a nominated key is pressed. |
| `InputPipeline.hpp` | `InputPipeline`, `InputPipelineOptions` | Assembles those decorators in the right order. |
| `InputState.hpp`, `Keyboard.hpp`, `Mouse.hpp` | — | Folding edges into held state, for a caller that wants it. |
| `ActionMap.hpp`, `Binding.hpp` | `ActionMap`, `Binding` | Naming a `Key` or `MouseButton` as an action. |
| `NullInputBackend.hpp`, `SelectedInputBackend.hpp` | — | The headless backend, and build-time selection. |
| `InputError.hpp` | `InputError` | A bad payload, or a key or button name nothing goes by. |

A conformance suite lives under `tests/conformance/`; `FakeInputBackend`, `MockInputBackend` and `MockInputEventCodec` are the test doubles.

## Depends on

[`engine`](engine.md) (for `StopOnKeySource`, which names `engine.stop`), [`event`](event.md), [`log`](log.md), [`replay`](replay.md) (every source it offers is an `IReplaySource`), [`time`](time.md).
It does **not** depend on [`gfx`](gfx.md) — no file here includes a `<antwika/gfx/...>` header or names a `gfx::` type — even though `gfx` is in its transitive link set via `replay`.
That is also why `input::Position` duplicates `gfx::Point` rather than reusing it, and why an input event does not say which window it arrived at.

## Non-obvious decisions

**Every event is an edge, never a state.**
A press, a release, a move, a notch — never "this button is currently down".
That is what lets a queue-based framework (SDL) and a state-polling one (raylib, which synthesises edges by diffing) implement the same interface.
An application that wants held state derives it from the edges, which is what `InputState` does.

**Persisted names are symbolic, never scancodes.**
`InputEventCodec` writes `"key": "Escape"`, not a platform keycode, so a session recorded under one backend replays under another.
No `input.*` name may ever be added to an app's self-generated-event list, because these are the events a replay exists to carry.

**Thinning happens upstream of the recorder, and only there.**
`CoalescingPointerSource` and `IdleMotionSource` sit before `TickEventRecorder`, so the file always holds exactly what the run consumed; doing it after the recorder would make the file disagree with the run, and doing it in a backend would hide it behind the seam.
`IdleMotionSource` latches the last held-back movement and releases it immediately *ahead* of the first event that could read a position, because `input.pointer_scroll` carries no position of its own and a zoom must anchor on the folded one.
Which decorator an app attaches is an app-level choice: [`game`](../apps/game.md) takes both, [`life`](../apps/life.md) takes only the gate, because a drag toggles every cell it crosses and coalescing a run inside a tick would skip some.
An app attaching the gate cannot draw anything that follows a free-moving pointer, since the movements between clicks are deliberately not in the tick stream.

**One queue, one drainer.**
SDL drains a single process-global queue for windows *and* input, so `backends/sdl3` owns a reference-counted `Sdl3Pump` shared by both of its targets, which calls `SDL_PollEvent` once and routes each event to the right queue.
Naming two different real frameworks for graphics and input is refused at configure time for the same reason.
