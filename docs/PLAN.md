# PLAN: Entity-Component-System (`antwika::ecs`)

Scaffolding doc per `REQUIREMENTS.md`'s "Could have" section.
Temporary — delete once the ECS lands and its design is either
self-evident from the code or written up under `blog/`.

## 1. Goal

Add a generic, reusable ECS library at `src/libs/ecs` (the `libs/ecs`
referred to in the request — every existing library lives under
`src/libs/<name>`, so this follows that convention exactly).

The library owns the *mechanism* (entities, component storage,
systems, phases, double buffering). It has no opinion about what any
concrete component or system means — that stays in application code
(`src/apps/game`, or future apps), matching the existing rule that
"state representation must remain an application concern; the engine
core must stay domain-agnostic."

## 2. Non-goals

- No archetype/chunk storage, no multithreaded scheduling, no
  reflection/serialization framework. A sparse-set store and a
  single-threaded, deterministic scheduler are enough for this
  project's scale and its determinism requirement.
- No entity-index recycling (see §3.1) — a deliberate simplification.
- No RNG, per the project-wide "Won't have."
- `antwika::ecs` will not depend on `antwika::engine`, `antwika::event`
  or `antwika::replay`. Wiring the ECS into the tick loop is an
  application concern (see §5), the same way `GameStateReducer` wires
  the reducer pattern into `Game::bootstrap` today. It *does* depend
  on `antwika::log` — see §3.1 for why, and note that `antwika::log`
  itself depends only on `antwika::time`, so this stays a small,
  domain-agnostic dependency chain, not a back door into `event`.
- Not replacing the existing event/reducer pattern — the ECS is an
  additional, independent tool an app can use for entity-shaped state
  instead of (or alongside) a hand-rolled struct + reducer. §6 covers
  a related but separate idea: generalizing the reducer pattern
  itself into its own small library.

## 3. Core design

### 3.1 Entity

```cpp
enum class Entity : std::uint64_t {};
inline constexpr Entity kNullEntity{};
```

A scoped enum over `std::uint64_t` with no enumerators: still a
primitive integer (trivially copyable, zero overhead, gets `==`/`<=>`
for free), but distinct from a bare `uint64_t` so it can't silently
mix with an unrelated integer. `0` is reserved for `kNullEntity`; real
entities start at `1`.

**No recycling.** `EntityManager::create()` hands out the next value
from a monotonically increasing counter and never reuses one, even
after `destroy()`. This drops an entire generation-counter/ABA-guard
subsystem the previous draft of this plan carried — simpler, and
still deterministic, since "never reuse" removes the one thing a
recycling scheme exists to make safe (a stale handle silently
aliasing a new entity can't happen if indices are never reused).
`destroy(Entity)` just flips an alive flag; the index itself is
retired for good.

**Exhaustion.** The counter's range (`std::uint64_t`) makes running
out practically impossible, but the behavior is still specified
exactly, per explicit instruction: if `create()` would need to hand
out a value past the configured ceiling (defaults to
`std::numeric_limits<std::uint64_t>::max()`, overridable in the
constructor purely so a test can force exhaustion in a handful of
calls instead of 2^64), `EntityManager` logs at `Level::Fatal` through
an injected `antwika::log::ILogger&` and terminates the process
(`std::exit(EXIT_FAILURE)`, after the log call, so buffered output
still flushes) — **not** an `EcsError`. This is a deliberate, narrow
exception to the "one specific, catchable error type" convention
used everywhere else in this plan (§3.8) and elsewhere in the
codebase (`ReplayFormatError`): exhausting the entity space isn't a
recoverable condition an app could sensibly catch and continue past,
so it doesn't pretend to be one. `Level::Fatal` already exists in
`antwika::log::Level`; this would be the first thing in the codebase
that actually acts on it instead of just logging it.

This is why `antwika::ecs` gains a dependency on `antwika::log` (see
§2) — a small, motivated, one-directional addition, not a sign the
library is creeping into domain territory.

`World` takes the same `ILogger&` it forwards to its `EntityManager`,
the same constructor-injection shape `Engine` already uses.

### 3.2 Component

```cpp
template <typename T>
concept Component =
    std::is_trivially_copyable_v<T> && std::is_standard_layout_v<T>;
```

A `concept`, not a marker base class — components stay plain
structs, as required. It's a compile-time constraint, checked
wherever `ComponentStorage<T>` or `World::add<T>` is instantiated.

### 3.3 Component storage and double buffering

This is the important part, since it's the mechanism that keeps
systems from mutating state directly.

Each component type `T` gets its own `ComponentStorage<T>`: a sparse
set (dense `vector<T>` + dense `vector<Entity>` + sparse index map)
holding **two** parallel value arrays, `front` and `back`.

- `World::get<T>(Entity) const` always reads `front`.
- `World::set<T>(Entity, T)` (or a mutable-reference variant) always
  writes into `back`.
- A system can therefore never observe a write — its own or a sibling
  system's — made earlier in the same synchronization window. It only
  ever sees `front`.
- At a commit point, every storage swaps `front`/`back`, then reseeds
  the new `back` as a copy of the new `front`, ready for the next
  window.

Structural changes (`World::create`, `destroy`, `add<T>`, `remove<T>`)
are queued, not applied immediately, and are replayed at the same
commit point, after the swap — so a view being iterated mid-phase is
never invalidated underneath a system.

### 3.4 Phases and the commit point — why they're the same mechanism

The request calls out two requirements that turn out to be one
mechanism: "systems should not directly affect game state, use double
buffering" and "systems may be attached to phases if ordering is a
necessity."

If a commit only ever happened once per tick, phase ordering would be
pointless — a "physics" phase could never see what an "input" phase
wrote earlier in the same tick, since `front` wouldn't update until
the tick ends. So the commit point is **per phase**, not per tick:

```
for each phase, in the order phases were created:
    for each system in that phase, in registration order:
        system.update(world, tick)   // reads front, writes back
    world.commit()                    // swap + apply structural changes
```

Within a phase, every system reads the identical `front` snapshot
taken at the start of that phase — this is the "no direct mutation"
guarantee. Across phases, each phase sees the accumulated result of
every phase before it in the same tick — this is what makes phase
ordering meaningful. A single-phase app just gets one commit per tick,
which degenerates to plain double buffering with no visible phase
concept, so simple apps pay nothing for the feature.

### 3.5 Views (queries over component combinations)

```cpp
template <Component... Ts>
class View { /* iterate entities that have all of Ts... */ };

template <Component... Ts>
View<Ts...> World::view();
```

Built by walking the dense array of whichever `Ts...` storage is
currently smallest and filtering by `has<T>()` on the rest — read-only,
always against `front`.

Iteration order determinism: dense arrays preserve insertion order.
Removal is a stable erase (shift, not swap-and-pop), which is O(n)
but keeps iteration order independent of *when* something else was
removed — a deliberate determinism-over-throughput trade-off,
consistent with this project's stated priorities (nothing in
`REQUIREMENTS.md` asks for ECS throughput; determinism is explicit).

### 3.6 Systems

```cpp
class ISystem
{
public:
    virtual ~ISystem() = default;
    virtual void update(World &world, antwika::time::Tick tick) = 0;
};
```

One small interface, same shape as `IEventSink`/`IEngine` elsewhere
in the codebase. Concrete systems are defined entirely by application
code and only ever get a `World&` — the type itself makes "read
front, write back" the only thing a system *can* do; there's no API
that hands out a mutable reference into `front`.

### 3.7 Phase scheduler

```cpp
using PhaseId = std::uint32_t;

class SystemScheduler
{
public:
    PhaseId createPhase(std::string_view name);
    void addSystem(PhaseId phase, ISystem &system);
    void run(World &world, antwika::time::Tick tick);
};
```

Phases are identified by creation order, not by name comparison —
`name` exists for logging/debugging only. No `enum class` imposed by
the library, since the set of phases is an application decision (an
app that needs no ordering just creates one phase).

### 3.8 Errors

One specific, catchable error type, `EcsError` (thrown by
`World::get<T>` on a missing/dead entity or absent component, and by
`World::destroy`/`remove<T>` on a dead entity) — mirrors the existing
`ReplayFormatError` precedent rather than inventing a new pattern.

Entity-ID exhaustion (§3.1) is the one deliberate exception: it logs
fatal and terminates instead of throwing `EcsError`, because it isn't
a condition an app could sensibly recover from.

## 4. File layout

```
src/libs/ecs/
├── CMakeLists.txt
├── include/antwika/ecs/
│   ├── Entity.hpp            // Entity, kNullEntity
│   ├── Component.hpp         // Component concept
│   ├── EcsError.hpp
│   ├── ComponentStorage.hpp  // template, header-only
│   ├── View.hpp              // template, header-only
│   ├── ISystem.hpp
│   ├── Phase.hpp              // PhaseId
│   ├── SystemScheduler.hpp
│   └── World.hpp
├── src/
│   ├── EntityManager.hpp/.cpp     // non-template, private to the lib;
│   │                               // monotonic counter + alive flags,
│   │                               // no free list
│   ├── IComponentPool.hpp         // private type-erased base, not
│   │                               // installed — same pattern as
│   │                               // replay's ReplayFormat.hpp
│   ├── World.cpp                  // structural-change queue, commit
│   │                               // orchestration across pools
│   └── SystemScheduler.cpp
└── tests/
    ├── CMakeLists.txt
    ├── EntityManagerTest.cpp
    ├── ComponentStorageTest.cpp
    ├── DoubleBufferingTest.cpp   // writes invisible to same-phase
    │                              // systems, visible after commit
    ├── ViewTest.cpp
    ├── WorldTest.cpp
    ├── SystemSchedulerTest.cpp   // phase/registration ordering
    ├── EcsDeterminismTest.cpp    // same input twice -> identical
    │                              // final World state
    └── mocks/
        └── include/antwika/ecs/mocks/MockSystem.hpp
```

`EntityManager` is kept as a private header under `src/`, not
`include/`, the same way `replay` keeps `BinaryPrimitives.hpp` and
`ReplayFormat.hpp` internal — it's an implementation detail `World`
composes, not part of the public surface apps consume.

## 5. Integration with the rest of the engine

`antwika::ecs` depends on `antwika::time` (for `Tick`) and
`antwika::log` (for the fatal-exhaustion path in §3.1) — nothing else.
An application wires it into the tick loop itself, e.g. an app-owned
adapter — call it `EcsTickSystem` in `src/apps/game` — implementing
`ITimedEventSink`, the same extension point `GameStateReducer`
already uses, that calls `scheduler.run(world, tick)` when it sees
`engine::events::kTick`. The `World` it wraps needs the same
`ILogger&` the app already constructs for `Engine`/`Logger` — no new
collaborator to wire up, just reused.

Whether the game app actually adopts the ECS for `GameState` (instead
of, or beside, the reducer) is a separate decision for later, once
the library exists — out of scope for this plan. A minimal usage
example belongs in the library's own tests, not necessarily in the
game app.

## 6. Related: generalizing the reducer pattern (`antwika::reducer`)

Worth asking, since the ECS and the existing hand-rolled
`GameStateReducer` both boil down to "fold tick-stamped events into
some private state and hand the engine an `ITimedEventSink`": should
that folding mechanism itself be a reusable library, so ECS state and
plain-struct state both plug into the engine the same way?

**The integration point already exists and is already generic**:
`antwika::event::ITimedEventSink` doesn't know or care what state it
folds events into — `GameStateReducer` is just the one implementation
that exists today. So this isn't "invent a new abstraction," it's
"stop hand-rolling the mutate-in-place plumbing per app-state type."

Proposed: a new small library, `src/libs/reducer`, depending only on
`antwika::event`:

```cpp
// IReducer<State>: a pure function of (previous state, event) ->
// next state. No mutation, no side effects — which is the same
// "systems don't directly touch state" discipline §3.3/§3.4 apply to
// the ECS, now applied to plain-struct state too.
template <typename State>
class IReducer
{
public:
    virtual ~IReducer() = default;
    virtual State reduce(const State &previous,
                          const antwika::event::TimedEvent &event)
        const = 0;
};

// ReducerSink<State>: the one generic adapter that makes any
// IReducer<State> pluggable into the engine as an ITimedEventSink.
template <typename State>
class ReducerSink final : public antwika::event::ITimedEventSink
{
public:
    ReducerSink(State &state, const IReducer<State> &reducer);
    void handle(const antwika::event::TimedEvent &event) override;

private:
    State &state;
    const IReducer<State> &reducer;
};
```

`GameStateReducer` becomes a pure `IReducer<GameState>`
implementation (returns a new `GameState` instead of mutating one in
place) plugged into a `ReducerSink<GameState>` — more testable
(assert on a return value, no shared mutable fixture needed) and a
worked example of the library for free.

The ECS does **not** route through `ReducerSink<State>` — `World`'s
internal double-buffer/commit lifecycle doesn't fit "return a whole
new state value" (copying every component of every entity every tick
just to satisfy a generic interface would be wasteful and pointless).
Instead, ECS keeps its own small `ITimedEventSink` adapter (§5). Both
are "pluggable into the engine as a reducer" in the sense that matters
— both terminate at the same `ITimedEventSink` seam — without forcing
the ECS through machinery shaped for a different problem.

Likely header-only (`IReducer`/`ReducerSink` are templates with no
state of their own), which would make `antwika::reducer` the first
`INTERFACE`-only library in this repo — a small, motivated deviation
from every existing library having real `.cpp` files, not a pattern
change for its own sake.

This is additive and independently sequenced from the core ECS work —
see §8's ordering. It doesn't block the ECS, and the ECS doesn't
depend on it.

## 7. Testing strategy

- GoogleTest + CTest, one behavior tested alongside the code that
  introduces it, per the project's existing rule.
- `DoubleBufferingTest`: a system that writes a component must not
  see that write when a second system in the *same* phase reads it;
  a system in the *next* phase (or the next tick) must see it.
- `EcsDeterminismTest`: build a `World`, register a fixed set of
  systems/phases, run N ticks from the same starting state twice,
  assert the resulting component values and entity iteration order
  are bit-identical — the same shape as the existing
  `ReplayDeterminismTest.cpp` in `src/libs/replay/tests`.
- `SystemSchedulerTest`: systems run in phase-creation order, then
  registration order within a phase, regardless of insertion order
  into any container.
- `EntityManagerTest`: index exhaustion is exercised with an injected
  low ceiling (not 2^64 real calls) via GoogleTest's `EXPECT_DEATH`,
  asserting the process terminates and the fatal message was logged
  through a mock `ILogger` beforehand.
- Every mock under `tests/mocks/include` must be used by at least one
  `.cpp`, per the project-wide rule already enforced elsewhere.

## 8. Step-by-step implementation order

See `docs/CHECKLIST.md` for the granular, checkable version of this.
High level:

1. Library scaffold (`CMakeLists.txt`, empty `include/src/tests`,
   wired into `src/libs/CMakeLists.txt`, linking `antwika::time` and
   `antwika::log`).
2. `Entity` + `EntityManager` (monotonic, no recycling, fatal
   exhaustion via injected `ILogger&`) (+ tests, incl. the death
   test).
3. `Component` concept + `EcsError` (+ tests).
4. `ComponentStorage<T>` with double buffering, no `World` yet
   (+ `DoubleBufferingTest` at the storage level).
5. `IComponentPool` type erasure + `World` (create/destroy/add/remove/
   get/set/commit), still no phases — single implicit commit per
   `World::commit()` call (+ tests).
6. `View<Ts...>` (+ tests).
7. `ISystem` + `SystemScheduler` with phases, wired to
   `World::commit()` per phase (+ `SystemSchedulerTest`,
   `EcsDeterminismTest`).
8. README update (add `ecs/` to the project-structure listing).
9. Optional: a small example system/component pair inside the
   library's own tests, purely to exercise the public API end to end
   — not a game-app integration, which is separate follow-up work.

Separate, non-blocking track (§6): `antwika::reducer` library, then
(optionally, as a demonstration) migrating `GameStateReducer` onto it.
Can happen before, after, or in parallel with 1–9 above — neither
track depends on the other.

## 9. Open questions

- Does `World::set<T>` take the whole value (copy-in) or hand out a
  mutable reference into `back`? Plan assumes copy-in (`set`) for a
  smaller, harder-to-misuse API; revisit if profiling ever says
  otherwise (not a concern raised anywhere in `REQUIREMENTS.md`).
- Should `SystemScheduler::run` commit once after the *last* phase
  even if that phase already committed, or is the last phase's commit
  sufficient? Plan assumes the latter — no double commit.
- Whether/when the game app itself adopts the ECS, and whether
  `GameStateReducer` actually gets migrated onto `antwika::reducer`
  (§6) or that stays a documented-but-unbuilt proposal, are both left
  open — separate decisions from landing the ECS library itself.
- `REQUIREMENTS.md` doesn't currently mention fatal/terminating error
  paths (its language centers on catchable errors) or a `reducer`
  library. Once §3.1's exhaustion behavior and/or §6 actually land,
  `REQUIREMENTS.md` likely wants a line for each — deliberately not
  edited now, since it documents current, built state, not proposals.
