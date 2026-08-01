# antwika::input

`src/libs/input/` — keyboard and pointer, as replayable edges.

## What it is for

Reading a keyboard and a pointer through an abstraction, and delivering what it reads into the tick loop as ordinary replay input.
As with [`gfx`](gfx.md), the concrete frameworks live under `backends/` beside the graphics ones, no file under `src/` names one, and which is compiled in is chosen at build time by `ANTWIKA_INPUT_BACKEND`.
Live input reaches the engine only through `ITickEventSource`, via `LiveInputSource`.

## Key types

| Header | Type | Role |
| --- | --- | --- |
| `IInputBackend.hpp` | `IInputBackend` | Polls a queue of `InputEvent`s and declares its `InputCapabilities`. |
| `InputEvent.hpp` | `InputEvent` | A `std::variant` of `KeyPressed`, `KeyReleased`, `PointerMoved`, `PointerButtonPressed`, `PointerButtonReleased`, `PointerScrolled`. |
| `Key.hpp`, `MouseButton.hpp`, `KeyModifiers.hpp` | `Key`, `MouseButton`, `KeyModifiers` | Symbolic names, with `toString()`. |
| `Position.hpp`, `Offset.hpp` | `Position`, `Offset` | Where and how far. |
| `Events.hpp` | `input::events::kKeyDown`, `kKeyUp`, `kPointerMove`, `kPointerDown`, `kPointerUp`, `kPointerScroll` | The event names, which are part of the replay file format. |
| `InputEventCodec.hpp`, `IInputEventCodec.hpp` | `InputEventCodec` | Encodes an `InputEvent` as an `event::Event` with a JSON payload, and back. |
| `LiveInputSource.hpp` | `LiveInputSource` | The `ITickEventSource` that puts live edges into the tick stream. |
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

[`engine`](engine.md) (for `StopOnKeySource`, which names `engine.stop`), [`event`](event.md), [`log`](log.md), [`replay`](replay.md) (for `PayloadJson`), [`simulation`](simulation.md) (every source it offers is an `ITickEventSource`), [`time`](time.md).
It deliberately does **not** depend on [`gfx`](gfx.md), and `gfx` does not depend on it: reading input does not require opening a window.
That is why `input::Position` duplicates `gfx::Point` rather than reusing it, and why an input event does not say which window it arrived at.

**That rule is about the source, not the link line**: no file under `src/libs/input` includes a `<antwika/gfx/...>` header or names a `gfx::` type, and that is the whole of what it forbids.
`gfx` is nevertheless *on* that link line, transitively: [`simulation`](simulation.md) links [`ecs`](ecs.md) and `gfx` for `TickPacer` and `WindowInputSource`, and this library links `simulation` because every source it offers is an `ITickEventSource`.
That was reviewed and accepted rather than overlooked — a linker seeing a library is not a dependency in the sense the rule cares about, since nothing in `input` can call into it — so a reader who finds `gfx` in `antwika_input`'s transitive link set has not found a violation.

## Non-obvious decisions

**Every event is an edge, never a state.**
A press, a release, a move, a notch — never "this button is currently down".
That is what lets a queue-based framework (SDL) and a state-polling one (raylib, which synthesises edges by diffing) implement the same interface.
An application that wants held state derives it from the edges, which is what `InputState` does.

**Persisted names are symbolic, never scancodes.**
`InputEventCodec` encodes an edge as an `input.*` event and writes `"key": "Escape"`, not a platform keycode, so a session recorded under one backend replays under another.
These are the events a replay exists to carry, so nothing downstream may quietly drop one.

**What lands in a recording is decided by where the recorder sits rather than by a list of names it skips.**
`event::TickEventRecorder` is an ordinary `ITickEventSink` an app registers on its `TickedEventDispatcher`, and it records unconditionally, so what it sees is exactly what an `ITickEventSource` supplied for that tick.
Never `engine.tick`, which `Engine::step()` dispatches for itself after `EngineLoop` has drained the source, and never anything a sink derives further down the tick path, like the tile [`game`](../apps/game.md)'s `GridSink` lays from a click.
An app used to carry a `kSelfGeneratedEventNames` list for that and no longer does, which is the better arrangement rather than an accident: a filter keyed on names is a rule every new event kind has to remember to obey, where a recorder that reduces nothing has nothing to forget.

**Thinning happens upstream of the recorder, and only there.**
`CoalescingPointerSource` and `IdleMotionSource` sit before `TickEventRecorder`, so the file always holds exactly what the run consumed; doing it after the recorder would make the file disagree with the run, and doing it in a backend would hide it behind the seam.
`IdleMotionSource` latches the last held-back movement and releases it immediately *ahead* of the first event that could read a position, because `input.pointer_scroll` carries no position of its own and a zoom must anchor on the folded one.
Which decorator an app attaches is an app-level choice: [`game`](../apps/game.md) takes both, [`life`](../apps/life.md) takes only the gate, because a drag toggles every cell it crosses and coalescing a run inside a tick would skip some.

Both exist because a window system reports motion at its own rate rather than the app's — SDL will report several hundred movements a second into a run that ticks 25 times a second — so an unthinned `--record` file grows at the window system's rate and is mostly positions nothing ever read.

That rule governs the event stream, which is everything a replay has to reproduce, and there is exactly one thing deliberately outside it: the channel below.

**A free-moving pointer reaches an app on a channel that is not an event and is in no recording.**
`PointerHintChannel` holds one `PointerHint`, written once per tick by `PointerHintSource` and read through an accessor named `forRenderingOnly()`.
An app opts in by naming a channel in `InputPipelineOptions::pointerHint`; naming none attaches nothing, so an existing app records byte for byte what it recorded before.

**What is read off that channel may decide what is drawn, and nothing else.**
That is the entire safety condition, and it is the price of the channel existing.
A live run and its replay do not agree on the value, deliberately: a replay holds none of the motion between clicks, so replaying publishes only the positions its recorded events happen to carry.
Fold a hint into anything a replay reproduces and the two diverge silently, with the symptom nowhere near the line that caused it.

**That it is a value cell rather than a marked event is what makes that structural.**
A "do not record" event would still travel the dispatcher, so every sink an app owns would be handed it and the rule would become something each sink had to observe.
A value cell reaches a sink only if somebody passed that sink the channel in a `main.cpp`, in the open, next to the pipeline that publishes it — the move `game::UiOverlay` and `life::DragState` already make.

`PointerHintSource` returns its inner source's events unmodified, so a recording is a function of a stream it cannot touch — which is what makes attaching it free rather than merely cheap.
`InputPipeline` attaches it immediately outside `LiveInputSource`, so no thinning decorator can hide a movement from it, and it is attached on a replay run too so the two branches still differ only in whether a device is read.

**The gate and the channel are not alternatives.**
The gate thins the recording and publishes nothing; the channel publishes and thins nothing; an app that draws a hover wants both.
The gate's one documented cost was that a hover could not be drawn at all, so wanting both was a combination that was not previously available.
The consequence to hold in mind is that the channel runs *ahead* of the event stream — on the tick a gated movement arrives the channel already has it, while the stream will not carry it until the next press, wheel or key — which is the point of the channel and exactly why the two may never be mixed.

**A [`ui`](ui.md) hover is the general answer to the gate's cost rather than a per-app one.**
`app::hoverFrom()` reads a `std::optional<PointerHint>` as a `ui::HoverPointer`, which is the type `ui::applyHover()` takes and the only thing that reaches a picture, so any app drawing a UI gets a hover on approach without one byte entering a recording.
It is a second function beside `app::pointerFrom()` rather than one more field on it, because the two positions come from different places and mean different things, and keeping them apart in the type system is what stops one being passed where the other belongs.
See [`docs/hover-is-not-simulation.md`](../../docs/hover-is-not-simulation.md).

**One queue, one drainer.**
SDL drains a single process-global queue for windows *and* input, so `backends/sdl3` owns a reference-counted `Sdl3Pump` shared by both of its targets, which calls `SDL_PollEvent` once and routes each event into a window queue or an input queue.
That sharing belongs in `backends/` rather than in `src/` because a framework directory already owns that framework's global state: admitting the single queue in one place behind the abstraction keeps the two library seams independent, where lifting it into `src/` would make it a rule `gfx` and `input` had to cooperate on.
Naming two different real frameworks for graphics and input is refused at configure time for the same reason.

Starting SDL itself is separate again: `Sdl3Runtime` does `SDL_Init(0)` once for the process and each seam claims its own subsystem, so a build selecting sdl3 for [`sound`](sound.md) alone never asks for a display.

The raylib input backend reports a pointer and no keyboard, and says so through its capabilities rather than claiming a device whose events never arrive.
It synthesises its edges by diffing state, since raylib has no queue at all.
