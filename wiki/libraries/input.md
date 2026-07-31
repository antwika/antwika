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
| `PointerHintChannel.hpp` | `PointerHintChannel`, `PointerHint` | One value cell holding where the pointer is, read through `forRenderingOnly()`. |
| `PointerHintSource.hpp` | `PointerHintSource` | Publishes that hint once per tick; a pure observer of the stream. |
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

Both exist because a window system reports motion at its own rate rather than the app's — SDL will report several hundred movements a second into a run that ticks 25 times a second — so an unthinned `--record` file grows at the window system's rate and is mostly positions nothing ever read.

**A free-moving pointer reaches an app on a channel that is not an event and is in no recording.**
`PointerHintChannel` holds one `PointerHint`, written once per tick by `PointerHintSource` and read through an accessor named `forRenderingOnly()`.
An app opts in by naming a channel in `InputPipelineOptions::pointerHint`; naming none attaches nothing, so an existing app records byte for byte what it recorded before.

**What is read off that channel may decide what is drawn, and nothing else.**
That is the entire safety condition, and it is the price of the channel existing.
A live run and its replay do not agree on the value, deliberately: a replay holds none of the motion between clicks, so replaying publishes only the positions its recorded events happen to carry.
Fold a hint into anything a replay reproduces and the two diverge silently, with the symptom nowhere near the line that caused it.

**That it is a value cell rather than a marked event is what makes that structural.**
A "do not record" event would still travel the dispatcher, so every sink an app owns would be handed it and the rule would become something each sink had to observe.
A value cell reaches a sink only if somebody passed that sink the channel in a `main.cpp`, in the open, next to the pipeline that publishes it.

`PointerHintSource` returns its inner source's events unmodified, so a recording is a function of a stream it cannot touch — which is what makes attaching it free rather than merely cheap.
`InputPipeline` attaches it immediately outside `LiveInputSource`, so no thinning decorator can hide a movement from it, and it is attached on a replay run too so the two branches still differ only in whether a device is read.

**The gate and the channel are not alternatives.**
The gate thins the recording and publishes nothing; the channel publishes and thins nothing; an app that draws a hover wants both.
The consequence to hold in mind is that the channel runs *ahead* of the event stream — on the tick a gated movement arrives the channel already has it, while the stream will not carry it until the next press, wheel or key — which is the point of the channel and exactly why the two may never be mixed.

**One queue, one drainer.**
SDL drains a single process-global queue for windows *and* input, so `backends/sdl3` owns a reference-counted `Sdl3Pump` shared by both of its targets, which calls `SDL_PollEvent` once and routes each event to the right queue.
Naming two different real frameworks for graphics and input is refused at configure time for the same reason.

Starting SDL itself is separate again: `Sdl3Runtime` does `SDL_Init(0)` once for the process and each seam claims its own subsystem, so a build selecting sdl3 for [`sound`](sound.md) alone never asks for a display.

The raylib input backend reports a pointer and no keyboard, and says so through its capabilities rather than claiming a device whose events never arrive.
