# Replay System — Implementation Plan

## 1. Goal

Give the engine the ability to record every event that occurs during a run and
serialize that recording to a byte stream ("replay"), and to later deserialize
and feed that recording back into a fresh engine instance such that the engine
reaches **exactly** the same state as the original run, deterministically.

Requirements taken from the request (and follow-up discussion):

1. Events can be serialized to / deserialized from a replay.
2. Loading a replay must deterministically reproduce the same state.
3. At least one automated test asserts save→load→replay determinism.
4. **Everything** that happens during engine execution must be replayable —
   not just a subset of events.
5. The engine operates on a **fixed timestep**.
6. `Event` must be **extendable**: application code (e.g. `apps/game`, or a
   future game built on this engine) can define and use its own event kinds,
   while still being able to use the engine's own built-in ("common") events —
   and both must flow through the same recording/replay pipeline uniformly.
7. A small, concrete example shows how application-level **state** is
   represented and updated from events, in `apps/game`.
8. No RNG/PRNG — out of scope for this plan entirely (not even a reserved
   field for one later).
9. SOLID principles, small interfaces, unit tests written **alongside** each
   step, not deferred to the end.

## 2. Current state of the codebase (research notes)

- `src/libs/event`: `Event` is currently just `{ std::string name; }`. There is
  already `EventDispatcher` (fans a dispatched event out to `IEventSink`s and
  pushes it onto an `IEventQueue`), `EventQueue` (FIFO `std::deque`), and
  `EventRecorder`, which is **both** an `IEventSink` and an `IEventHistory` and
  already accumulates every dispatched event into a `std::vector<Event>`. This
  is 80% of an event-sourcing log already — it just isn't persisted, isn't
  tick-stamped, and isn't replayed back in.
- `src/libs/engine`: `Engine::start()` logs a line, then drains whatever is
  already sitting in the `IEventQueue` **once** and returns. There is no
  notion of a tick, a fixed delta-time, or a run loop. This needs to change
  for "fixed time" execution to mean anything.
- `src/libs/time`: `IClock` returns wall-clock `system_clock::time_point`,
  used today only for log timestamps (`Logger` via `PlainFormatter`). Wall
  time is already isolated to logging and must **not** leak into simulation
  state, or replays would stop being deterministic. `Tick` will be added here
  (see §3.1) — a step count is fundamentally a "how has time progressed for
  the simulation" concept, and `time` is already that lib's job.
- `src/apps/game`: `Game::run()` wires a dispatcher + engine together and
  calls `engine.start()`. `bootstrap()` free function assembles concrete
  collaborators. `main.cpp` builds concrete objects and calls `bootstrap`.
- Conventions observed across every existing lib (`log`, `time`, `event`,
  `engine`), which the plan below follows exactly:
  - `IThing.hpp` (pure virtual interface) in `include/antwika/<lib>/`, `Thing`
    concrete implementation `.hpp`/`.cpp` pair.
  - Namespace `antwika::<lib>`, deleted copy/move ctors on stateful classes,
    constructor-injected collaborators held by reference (no ownership,
    no `new`/smart pointers).
  - `tests/ThingTest.cpp` using GoogleTest, plus `tests/mocks/` (gmock,
    header-only `INTERFACE` library, aliased `antwika::<lib>::tests::mocks`)
    and/or `tests/fakes/` (hand-written fake, e.g. `FakeClock`).
  - Every mock/fake header must be `#include`d by at least one `.cpp` file or
    `scripts/check_unused_test_doubles.py` fails CI — so no speculative mocks.
  - Each lib is its own CMake target (`add_library(antwika_<lib> ...)`,
    aliased `antwika::<lib>`), added to `src/libs/CMakeLists.txt`.
  - No serialization/JSON dependency exists yet (`conanfile.py` only pulls in
    `gtest`). Adding one is a deliberate, callable-out decision — see §6.

## 3. Key design decisions

### 3.1 `Tick` lives in the `time` lib

```cpp
namespace antwika::time { using Tick = std::uint64_t; }
```

No new library. `engine`, `event`, and `replay` all already depend on (or will
depend on) `time`, so this adds no new dependency edges.

### 3.2 Extending `Event` without breaking it, and without a class hierarchy

`Event` stays a single, simple, value-semantic, serializable struct — no
polymorphism, no `unique_ptr<Event>` subclass hierarchy. Instead it grows an
opaque payload:

```cpp
struct Event
{
    std::string name{};
    std::string payload{};   // NEW: opaque, app-defined bytes. Empty for
                              // events that carry no data (most built-ins).
    bool operator==(const Event &other) const = default;
};
```

Why this shape, and not a base-class/virtual `Event` hierarchy:

- The engine's job (dispatch, queue, record, serialize, replay) never needs
  to understand *what an event means* — only that it has a name and some
  bytes. A closed, concrete `Event` keeps every one of those mechanisms
  simple, value-typed, and already-proven (copyable, `==`-comparable, stored
  by value in `std::deque`/`std::vector`, trivial to (de)serialize).
- **Open/Closed in practice**: adding a new event kind — whether it's a
  built-in the engine ships, or one `apps/game` (or any future app) invents —
  never requires touching `Event`, `EventQueue`, `EventDispatcher`,
  `EventRecorder`, or the `replay` lib's codec. It only requires: pick a
  `name`, decide how to pack/unpack `payload` (that encode/decode logic lives
  entirely in application code, e.g. `apps/game`), and dispatch it. This is
  the concrete mechanism behind requirement 6 ("extendable, app can create its
  own events, while still benefiting from common ones").
- **Common (built-in) events**: the engine gets to define a handful of its
  own event names as named constants (e.g.
  `antwika::engine::events::kTick = "engine.tick"`, see §3.3) so consumers
  don't hand-roll magic strings for engine-provided events, while
  application code is free to define its own equally-valid names
  (`"game.score_increment"`, etc.) in its own headers. Both travel through
  the exact same `Event`/`TimedEvent` type — there is no special-casing
  anywhere in the pipeline between "built-in" and "custom" events, which is
  precisely what makes them interchangeable from the replay system's point
  of view.
- A polymorphic hierarchy was considered and rejected for this codebase: it
  would require a type registry/factory for deserialization (`name → decode
  function`) to reconstruct concrete subclasses from bytes, adds
  ownership/lifetime complexity (`unique_ptr` + cloning to keep value
  semantics), and none of that complexity is actually needed to satisfy the
  requirement — an opaque payload gets the same extensibility with far less
  machinery, and matches the concrete-value-type style used everywhere else
  in this codebase today.

### 3.3 Fixed timestep requires the engine to own a tick loop, and gets its own built-in events

Today `Engine::start()` drains the queue once — there is no "fixed time"
without a loop that advances a tick counter by a fixed `Δt` each iteration.

- `IEngine` gains a step-oriented shape alongside the existing `start()`:
  ```cpp
  class IEngine {
  public:
      virtual ~IEngine() = default;
      virtual void start() = 0;          // unchanged: boot/log
      virtual void step(Tick tick) = 0;   // NEW: advance exactly one fixed tick
  };
  ```
- A new small orchestrator (`Simulation` or `EngineLoop`) owns the actual
  `for (tick = 0; tick < totalTicks; ++tick) { ...; engine.step(tick); }`
  loop. In a live run it dispatches externally-produced events (e.g. from
  `Game`) as they occur; in replay mode it instead pulls that tick's events
  from an `IReplaySource` (§3.6) and dispatches those. **The rest of the loop
  body is identical in both modes** — that symmetry is what makes "load
  replay → identical state" a provable property instead of a hope.
- `Engine::step(tick)` dispatches one of the engine's own built-in events —
  `events::kTick` (`"engine.tick"`) — through the `IEventDispatcher` before
  draining and processing that tick's queued events. This is the concrete
  "common event that comes with the engine" from requirement 6: any
  application-level state reducer (§3.7) can react to ticks passing without
  the application having to invent and manually dispatch its own tick event.
- Every event that becomes visible to the engine during a tick — whether it's
  `engine.tick`, a live-dispatched application event, or a replayed one —
  must be stamped with the current `Tick` before it reaches any sink. This is
  done once, centrally, by giving `EventDispatcher` the current tick (see
  §3.4), so *every* dispatched event is tick-stamped automatically —
  satisfying requirement 4 without every call site needing to remember to do
  it.

### 3.4 Tick-stamping: a decorator, not a change to the tested `EventDispatcher`

`EventDispatcher` already has passing tests (`EventDispatcherTest.cpp`,
`BootstrapTest.cpp`) that assert its exact current behavior. Rather than
modify it, add a `TickedEventDispatcher` that wraps an `IEventDispatcher`,
tracks/accepts the current `Tick`, and additionally fans each dispatched
event out to a set of `ITimedEventSink`s as a `TimedEvent{tick, event}`:

```cpp
struct TimedEvent { antwika::time::Tick tick; Event event; bool operator==(const TimedEvent&) const = default; };

class ITimedEventSink {
public:
    virtual ~ITimedEventSink() = default;
    virtual void handle(const TimedEvent &event) = 0;
};
```

This is Open/Closed applied to the existing, working `EventDispatcher`: it
and its tests are untouched; tick-awareness is added *around* it.

### 3.5 Recording: a tick-aware sibling of `EventRecorder`

```cpp
class ITimedEventHistory {
public:
    virtual ~ITimedEventHistory() = default;
    [[nodiscard]] virtual std::vector<TimedEvent> getEvents() const = 0;
};

class ReplayRecorder final : public ITimedEventSink, public ITimedEventHistory {
    void handle(const TimedEvent &event) override;
    std::vector<TimedEvent> getEvents() const override;
private:
    std::vector<TimedEvent> events;
};
```

`ReplayRecorder` mirrors `EventRecorder` exactly and lives next to it in the
`event` lib. It's registered as one of the `TickedEventDispatcher`'s timed
sinks, exactly like `EventRecorder` is registered as a plain sink today.

### 3.6 Serialization: codec + writer/reader, split by responsibility, new `replay` lib

```cpp
// How to turn ONE TimedEvent into bytes and back. Extending to new event
// payloads later never touches this class (Open/Closed) -- Event's payload
// is already opaque bytes, so this codec only needs to (de)serialize the
// envelope: tick, name, payload.
class IEventCodec {
public:
    virtual ~IEventCodec() = default;
    virtual void encode(const TimedEvent &event, std::ostream &out) const = 0;
    [[nodiscard]] virtual TimedEvent decode(std::istream &in) const = 0;
};

class IReplayWriter {
public:
    virtual ~IReplayWriter() = default;
    virtual void write(const std::vector<TimedEvent> &events, std::ostream &out) const = 0;
};

class IReplayReader {
public:
    virtual ~IReplayReader() = default;
    [[nodiscard]] virtual std::vector<TimedEvent> read(std::istream &in) const = 0;
};
```

- I/O is expressed as `std::ostream&`/`std::istream&`, mirroring the existing
  `StreamAppender(std::ostream&)` pattern in the `log` lib. This keeps the
  `replay` lib decoupled from the filesystem (DIP) — callers decide whether
  the stream is a `std::ofstream`, a `std::stringstream` (used heavily in
  tests), a network socket, etc.
- `BinaryEventCodec` (concrete `IEventCodec`) is the default implementation:
  fixed-width `Tick` (8 bytes, explicit byte order via `std::endian`/manual
  shifts — never native struct layout), then length-prefixed `name`, then
  length-prefixed `payload`. Dependency-free.
- `BinaryReplayWriter`/`BinaryReplayReader` wrap a codec and add a small
  versioned header: magic bytes (`"ARPL"`), format version (`uint32_t`), and
  an event count, followed by that many encoded `TimedEvent`s. Version +
  magic let `read()` throw a specific, typed exception
  (`ReplayFormatError`) on garbage/incompatible input instead of misbehaving
  silently — this gets its own unit tests (truncated stream, bad magic,
  unsupported version).
  > **Implementation note:** an earlier draft of this section reserved a
  > header field for the fixed timestep `Δt` a replay was recorded at, on
  > the theory that a replay file should be self-describing about its
  > playback rate. Building it surfaced that this engine has no wall-clock
  > playback rate at all — `Engine::step()` advances by a discrete `Tick`,
  > never by a duration — so the field would have had no consumer. Dropped,
  > same reasoning as the RNG-seed non-decision in §7. See
  > `docs/notes/09-serialization.md`.

### 3.7 Feeding a loaded replay back into the engine

```cpp
class IReplaySource {
public:
    virtual ~IReplaySource() = default;
    virtual std::vector<Event> eventsFor(antwika::time::Tick tick) = 0; // in recorded order
};

class ReplaySource final : public IReplaySource {
public:
    explicit ReplaySource(std::vector<TimedEvent> events);
    std::vector<Event> eventsFor(antwika::time::Tick tick) override;
private:
    std::vector<TimedEvent> events;
};
```

The tick-loop orchestrator (§3.3), in replay mode, asks `IReplaySource` for
tick *N*'s events, dispatches them through the same `TickedEventDispatcher` a
live run would have used, then calls `engine.step(N)`. Live mode and replay
mode differ **only** in where events for a tick come from.

### 3.8 State: suggested representation, kept out of the engine core

The engine core (`engine`, `event`, `replay` libs) is deliberately
**domain-agnostic** — it has no idea what "score" or "player" means, and
shouldn't. State is an application concern, so it's designed and demonstrated
at the `apps/game` layer, using a mechanism the engine already provides
rather than a new one:

**Recommended pattern: plain-data state + reducer(s) implementing
`ITimedEventSink`.**

```cpp
// apps/game — NOT part of the engine core.
struct GameState
{
    std::uint64_t ticksProcessed{};
    std::uint64_t score{};
    bool operator==(const GameState &) const = default;
};

class GameStateReducer final : public ITimedEventSink
{
public:
    explicit GameStateReducer(GameState &state);
    void handle(const TimedEvent &event) override;
    // on events::kTick            -> state.ticksProcessed++
    // on "game.score_increment"   -> state.score += decode(event.event.payload)
private:
    GameState &state;
};
```

Rationale / alternatives considered:

- **Why a reducer over `ITimedEventSink` rather than a new interface**: the
  engine already generalizes "something that reacts to a timed event" via
  `ITimedEventSink` — `ReplayRecorder` is one implementation (record it),
  `GameStateReducer` is another (fold it into state). No new abstraction is
  needed; this is Open/Closed and Liskov working together — anything that
  can legally stand in for an `ITimedEventSink` can be registered as a sink,
  recorder and reducer included, and the dispatcher doesn't need to know or
  care which.
- **Why plain data + a separate reducer, not a "smart" state object with
  methods per event**: keeps `GameState` trivially testable and comparable
  (`operator==`, same pattern already used for `Event`/`TimedEvent`) and
  keeps "what an event means" (reducer) separate from "what the data is"
  (state) — classic SRP split, and it's exactly the split that makes the
  determinism test in §5 step 12 meaningful: `GameState` equality after a
  live run vs. after a replayed run is the assertion.
- **Live incremental reduction now; snapshotting later if ever needed**: the
  reducer mutates `state` in place, tick by tick, as events arrive — no
  "replay from scratch to get current state" indirection. If a future need
  arises to jump to tick *N* without processing 0..N-1 one at a time (e.g. a
  scrub-through-replay UI), periodic snapshots of `GameState` can be added
  later as a pure optimization on top of the same reducer, without changing
  this design.
- This also demonstrates requirement 6 end-to-end: `GameState`/
  `GameStateReducer` react to the engine's own built-in `engine.tick` event
  *and* to `apps/game`'s own custom `"game.score_increment"` event, through
  the identical sink mechanism — proving "benefits from common events" and
  "can define its own" are the same code path, not two.
- Because `GameState` is just data, the `apps/game` end-to-end determinism
  test (§5 step 13) can assert `GameState` equality directly, which is a more
  meaningful, readable assertion than a hash. (The `replay` lib's own,
  lower-level determinism test — §5 step 12 — doesn't depend on `apps/game`
  at all, so it uses a small self-contained test-only reducer/state instead;
  see that step.)

## 4. New / changed components, by library

| Library | New/changed types |
|---|---|
| `time` | `Tick` (new alias) |
| `event` | `Event` gains `payload` (`std::string`); `TimedEvent` (new); `ITimedEventSink`, `ITimedEventHistory` (new); `TickedEventDispatcher` (new, decorates `IEventDispatcher`); `ReplayRecorder` (new, next to `EventRecorder`) |
| `engine` | `IEngine::step(Tick)` added; `Engine` reworked to process one fixed tick per call, dispatching its own built-in `events::kTick` event first |
| `replay` (new lib) | `IEventCodec`, `BinaryEventCodec`; `IReplayWriter`, `BinaryReplayWriter`; `IReplayReader`, `BinaryReplayReader`; `IReplaySource`, `ReplaySource`; a small `EngineLoop`/`Simulation` orchestrator; a replay-format error type |
| `apps/game` | `GameState`, `GameStateReducer` (example state representation, §3.8); a custom `"game.score_increment"` event; `bootstrap()` gains record/replay entry points; `main.cpp` gains `--record <file>` / `--replay <file>` flags |

Every new interface gets a mock under `tests/mocks/` (or fake under
`tests/fakes/`) **only when a test actually needs one to land** — per
`scripts/check_unused_test_doubles.py`, no speculative test doubles.

## 5. Step-by-step implementation plan

Every step below compiles on its own, ships its own unit test(s), and ends
with a commit (see §8, Process). No step should leave the tree red or push
testing to "later."

1. **`Tick` in `time` lib.** Type alias only; no behavior. No dedicated test
   beyond it compiling and being usable — folded into the first test that
   consumes it (step 2).
2. **`Event` gains `payload`.** Update the struct, confirm `operator==`
   still round-trips via a quick `EventTest.cpp` case (empty payload,
   non-empty payload, two events differing only by payload are unequal).
   Existing call sites are source-compatible (`payload` defaults to `""`).
3. **`TimedEvent` value type + tests.** `operator==`, tick/event field
   access. `TimedEventTest.cpp`.
4. **`ITimedEventSink` / `ITimedEventHistory` + `ReplayRecorder`.** Mirrors
   `EventRecorder`. `ReplayRecorderTest.cpp` mirrors `EventRecorderTest.cpp`:
   handle two timed events, assert `getEvents()` returns them in order.
5. **`TickedEventDispatcher`.** Wraps an `IEventDispatcher`, exposes a way to
   set/advance the current tick, forwards to the wrapped dispatcher *and* to
   registered `ITimedEventSink`s as `TimedEvent{tick, event}`.
   `TickedEventDispatcherTest.cpp` using a `MockEventDispatcher` (already
   exists) + a new `MockTimedEventSink`: assert both the wrapped dispatcher
   and the timed sink receive the right calls, in sequence, for multiple
   ticks.
6. **`events::kTick` + built-in event constants.** A small header of named
   constants in `engine` (e.g. `antwika/engine/Events.hpp`) so "engine.tick"
   isn't a magic string duplicated between engine code and its consumers.
7. **`IEngine::step(Tick)` + fixed-timestep `Engine`.** Rework `Engine` to
   take a `TickedEventDispatcher` (in addition to logger/queue), dispatch
   `events::kTick` at the start of `step()`, then process exactly that tick's
   queued events. Update `EngineTest.cpp`: empty tick, single event, multiple
   events in one tick, and — the first, cheapest determinism check, no
   serialization involved — two fresh engines fed the identical live event
   sequence across the identical tick count producing an identical
   `ReplayRecorder` history.
8. **Tick-loop orchestrator (`EngineLoop`/`Simulation`).** Drives
   `engine.step(tick)` for `tick in [0, totalTicks)`, dispatching either
   live-supplied events or (in replay mode) events pulled from an injected
   `IReplaySource` before each step. Unit test with `MockEngine`: `step()`
   called once per tick, in order (`InSequence`, matching the existing style
   in `EngineTest.cpp`).
9. **`replay` lib scaffolding.** New `src/libs/replay/{include,src,tests}`,
   `CMakeLists.txt` following the `event` lib template, registered in
   `src/libs/CMakeLists.txt`. Empty but buildable — later steps are pure
   add-a-file diffs. (No behavior to test yet; this step's "test" is a green
   build.)
10. **`IEventCodec` / `BinaryEventCodec` + tests.** Round-trip a
    `TimedEvent` through a `std::stringstream`. Edge cases: empty
    `name`/`payload`, non-ASCII UTF-8 bytes in either field, tick `0`, tick
    `UINT64_MAX`.
11. **`IReplayWriter`/`IReplayReader` + `BinaryReplayWriter`/
    `BinaryReplayReader` + tests.** Round-trip a `vector<TimedEvent>` (0, 1,
    N events) via `std::stringstream`. Negative tests: truncated stream, bad
    magic bytes, unsupported version — each throws a specific, documented
    exception type.
12. **`IReplaySource`/`ReplaySource` + a self-contained determinism test.**
    `eventsFor(tick)` returns exactly the events recorded at that tick, in
    order. Then, **the determinism test requirement (2/3) at the `replay`
    lib's level**, using a small test-local reducer/state (no dependency on
    `apps/game`):
    - Run A: drive the tick loop for N ticks with a scripted live event
      sequence through a real `Engine` + `ReplayRecorder` + a trivial
      test-only reducer (e.g. folds `(tick, name, payload)` into a
      `std::uint64_t` via FNV-1a — defined in the test file itself, it's not
      production code).
    - Serialize Run A's `ReplayRecorder::getEvents()` via
      `BinaryReplayWriter` into an in-memory `std::stringstream`.
    - Deserialize via `BinaryReplayReader` into a `ReplaySource`.
    - Run B: fresh `Engine`, same N ticks, sourced from the `ReplaySource`
      instead of live input.
    - Assert Run A's and Run B's folded state are equal, **and** their
      `ReplayRecorder` histories are element-wise equal.
    - A second test serializes Run A's recording twice and asserts the two
      byte buffers are byte-for-byte identical (guards against accidentally
      introducing nondeterministic serialization, e.g. an unordered
      container).
13. **`apps/game` state example + end-to-end demo/test.** Add `GameState` +
    `GameStateReducer` (§3.8) and a custom `"game.score_increment"` event;
    wire the reducer into `bootstrap()` as a registered timed sink. Add
    `GameStateReducerTest.cpp` (unit-level: feed synthetic `TimedEvent`s,
    assert `GameState` fields). Then an app-level record/replay test
    (`src/apps/game/tests/ReplayIntegrationTest.cpp`): run the game briefly
    (a few ticks, including a score-increment event), record, serialize to
    an in-memory stream, deserialize, replay into a fresh `Game`/`GameState`,
    assert the two `GameState`s are equal. This is the test that most
    directly demonstrates requirements 2, 3, 6, and 7 together, in the
    actual application.
14. **Wire record/replay into `main.cpp`.** Minimal `--record <file>` /
    `--replay <file>` argv handling (no new dependency); record path writes
    the `ReplayRecorder` history to a real file at shutdown, replay path
    reads a real file into a `ReplaySource`. Covered indirectly by step 13's
    stream-based test; a thin `main`-level smoke test is optional and not
    required to prove determinism (that's already proven at lower levels).
15. **Docs.** Update `README.md`'s project-structure tree to include
    `replay/`, and add a short "Replays" section describing the
    `--record`/`--replay` flags and, briefly, the state/reducer pattern for
    anyone extending the game with new events.

## 6. Dependency decision

No new third-party dependency is required — the binary codec in §3.6 uses
only `<cstdint>`, `<ostream>`, `<istream>`. This keeps `conanfile.py`
untouched. If a human-readable format (e.g. JSON) is ever preferred for
debugging/tooling, `IEventCodec`/`IReplayWriter`/`IReplayReader` were
specifically split out so a `JsonEventCodec` could be added later without
touching the rest of the pipeline (Open/Closed).

## 7. Decisions made during planning (for the record)

These were open questions in the first draft of this plan; they're resolved
now so implementation isn't blocked:

- **`Tick` location**: `time` lib, not a new `simulation` lib (per your
  steer). Revisit only if a real second simulation-specific type shows up.
- **Event extensibility**: opaque `payload: std::string` on the existing
  concrete `Event`, not a polymorphic hierarchy — see §3.2 for the full
  rationale.
- **Tick-aware dispatch**: a decorator (`TickedEventDispatcher`) wrapping the
  existing `IEventDispatcher`, rather than modifying it — keeps the
  already-tested `EventDispatcher` and its tests untouched (§3.4).
- **`ReplayRecorder` location**: `event` lib, next to `EventRecorder` (its
  closest sibling and near-identical shape), not the `replay` lib. `replay`
  depends on `event` regardless (for `TimedEvent`).
- **RNG/PRNG**: explicitly out of scope. No field reserved for a seed in the
  replay header — if/when determinism-sensitive randomness is needed later,
  that's a deliberate future format change, not something to pre-guess now.
- **State representation**: plain-data struct + `ITimedEventSink`-based
  reducer(s), demonstrated in `apps/game`, not baked into the engine core
  (§3.8).

No blocking open questions remain.

## 8. Process (working agreement for implementation)

- Tests are written **alongside** each step in §5, not deferred — a step
  isn't "done" until its behavior is asserted by a test that fails without
  the implementation and passes with it.
- Once a step (or a natural sub-chunk of one) is complete and green
  (build + tests + `scripts/check_unused_test_doubles.py`), make a git
  commit for it. Small, frequent commits over one large one.
- Commit messages follow [Conventional Commits](https://www.conventionalcommits.org/)
  (`feat: ...`, `test: ...`, `refactor: ...`, `docs: ...`, etc.), matching
  this repo's existing history style (e.g. `fix: adjust badge generation...`,
  `docs: clarify coverage badge metrics`).
- No pushing to any remote — commits stay local until explicitly asked for.

## 9. Risks / determinism pitfalls to guard against as this is built

- Any future code that reads `IClock::now()` (wall time) into simulation
  state, rather than only into log timestamps, breaks determinism instantly.
- Iterating an unordered associative container (e.g. if sinks or event
  payloads ever move to a `std::unordered_map`) is a classic hidden source
  of platform-dependent ordering — stick to `std::vector`/`std::deque`
  (already the case throughout the codebase today).
- Floating-point determinism across compilers/platforms (GNU vs LLVM vs
  MinGW — all three are already CI targets per the dev containers) is a
  known hard problem if/when physics or float-heavy gameplay state is added.
  Out of scope for this plan (no floats in state yet) but worth flagging
  since the project already builds on three toolchains.
