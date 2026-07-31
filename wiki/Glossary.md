# Glossary

**Tick** — the simulation's unit of time, `antwika::time::Tick`, an unsigned counter.
The engine advances one tick at a time and never sees a wall-clock delta, which is what makes a run independent of how fast the machine was.

**Event** — `antwika::event::Event`, a `name` and an opaque string `payload`.
Names are dot-namespaced by whoever owns them: `engine.tick`, `input.pointer_down`, `game.score_increment`, `life.toggle_cell`, `task.submit`, `poker.buy_in`.
There is one mechanism, with no distinction between built-in and application-defined kinds.

**TickEvent** — an `Event` bound to the tick it occurred on.

**Dispatcher** — an `IEventDispatcher` that fans an event out to registered sinks.
`TickedEventDispatcher` is the one the loop uses; it stamps the current tick on.

**Sink** — an `ITickEventSink`: something that observes events and does something with them.
This is where an application turns input into meaning — a click into a placed tile, a payload into a scheduled job.
A sink runs inside the tick path, downstream of the recorder, which is what lets a replay store the input and regenerate what it caused.

**Tick source** — an `ITickSource`, asked `eventsFor(tick)` once per tick.
This is the only seam between a live run and a replayed one: a replayed run is served by `ReplaySource` from a file, a live run by `input::LiveInputSource` over a backend.

**Decorator (of a source)** — a source wrapping another source to add or remove events: `WindowInputSource`, `WindowCloseSource`, `StopOnKeySource`, `CoalescingPointerSource`, `IdleMotionSource`.
Anything that thins a recording must be a decorator upstream of the recorder, never a filter applied afterwards.

**Recorder** — `TickEventRecorder`, which collects what passed through for a `--record` run.
It is the only place a reduction of what is recorded may happen.

**Replay** — a JSON document of the external input a run consumed, written by `ReplayWriter` and read by `ReplayReader`.
It holds only what the engine cannot regenerate: never `engine.tick`, never a placement derived from a click, never a card dealt from a seeded shuffle.

**Self-generated event** — an event the engine or the application regenerates deterministically each run, and therefore never records.
Each app declares its own list; no `input.*` name may ever appear in one.

**Engine loop** — `simulation::EngineLoop`, the one code path both live and replayed runs go through.

**Reducer** — an `ITickEventSink` that folds each event into a state value it owns, such as `game::GameStateReducer`.
It is a naming convention rather than a library: nothing in `src/libs/` defines the term.

**Entity / component / world** — the [`ecs`](libraries/ecs.md) alternative to a reducer fold.
An `Entity` is an opaque id, a component is a value attached to one, and the `World` is double-buffered so mutations are staged and become visible at a commit between ticks.

**System** — an `ecs::ISystem`, `update(World &, Tick)`, run once per tick by a `SystemScheduler` in `Phase` order.

**Backend** — a concrete framework implementation under `backends/`, chosen at build time and statically linked.
Graphics backends implement `gfx::IGfxBackend`, input backends `input::IInputBackend`; `null`, `sdl3` and `raylib` exist.
The `null` pair is headless: it needs no framework, draws nothing and reports no input.

**Conformance suite** — the shared test body every backend of a kind must pass, so the abstraction means the same thing under each.

**Snapshot** — an immutable value describing state for drawing, handed to a scene: `game::SceneSnapshot`, `poker::TableSnapshot`.
It is the structural half of "rendering is write-only" — a scene is given a value it cannot write back through.

**Scene** — the code that turns a snapshot into drawing calls, e.g. `game::GridScene`, `poker::TableScene`, `life::BoardScene`.

**Canvas** — the window size a UI or a board layout is computed against.
It is always the size the window was *asked* for, never the size a window reports, so a recorded click resolves to the same widget or cell under any backend.

**Edge (of input)** — a press, a release, a movement or a scroll notch, as opposed to a statement of what is currently held.
Every `InputEvent` is an edge, which is what lets a queue-based framework and a state-polling one implement the same interface.

**Job / budget** — a [`scheduler`](libraries/scheduler.md) unit of work and the cap on how many run in one `run()` call.
`budget` is the only throttle; no job runs outside a `run()`.

**Frame pass** — an `app::IFramePass`, drawn in the gap between two ticks by `app::FramePacedSource`.
It is handed an `animation::Progress` and no `World`, no `Tick` and no dispatcher, so a pass cannot change what the simulation computes.

**Progress** — `animation::Progress`, an exact rational position within something: a numerator and a denominator rather than a float, so the same frame is the same pixel on every toolchain.

**Pointer hint** — `input::PointerHint`, carried on a channel that is not an event and is in no recording.
What is read off it may decide what is *drawn* and nothing else, because a live run and its replay deliberately disagree on its value.

**Footprint** — the block of cells a building covers, from a table keyed by its kind rather than a field on the component.
Restricted to squares, because a square block projects to the same 2:1 diamond one atlas tile already is.

**Frame (of audio)** — one sample per channel, at one instant.
A render callback is told the **absolute** index of its first frame, counted from when the device started, which is what makes a scheduled sound land where it was placed.

**Pumped (of a device)** — rendered only when a caller asks, on the caller's own thread, as opposed to *self-driven* on a thread the framework owns.
Every sound device in the project is pumped, which is why the project has no second concurrency model.

**Domain (in WFC)** — the set of values a cell may still take, narrowed by constraints until one remains.

**Hand value** — `holdem::HandValue`, a single comparable number a 5–7 card hand evaluates to; greater is stronger and equal is a split pot.
