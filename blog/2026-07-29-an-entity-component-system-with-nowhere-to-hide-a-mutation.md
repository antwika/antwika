# An entity-component-system with nowhere to hide a mutation

*2026-07-29*

The [previous post](2026-07-27-building-a-deterministic-replay-system.md) ended with a promise: state is an application concern, the engine core stays domain-agnostic, and `apps/game`'s hand-rolled `GameState` + `GameStateReducer` was "a small example," not the only shape state is allowed to take.
This post is about the other shape: an entity-component-system, added as a new library, `antwika::ecs`, that applications can build on instead of (or alongside) a plain struct and a reducer.
It's also about a request that showed up mid-implementation and reshaped a chunk of the design for the better, and about two places where the original plan simply didn't compile once real code had to satisfy it.

## Requirements

Distilled from the actual ask:

1. Entities are plain integers, or something close enough to one.
2. Components are plain structs — data, no behavior.
3. Systems operate on combinations of components.
4. The library is abstract; applications supply their own concrete components and systems.
5. Systems must not directly mutate state — some form of double buffering is required instead.
6. Systems can be attached to phases when ordering matters.
7. Modern C++, and everything about it deterministic.

Two more requirements arrived after the plan was already written, mid-conversation, not from re-reading the brief but from a reviewer's actual questions:

8. No entity-index recycling — exhausting the index space logs a fatal error and terminates the process, rather than pretending that's a recoverable condition.
9. Consider generalizing the existing reducer pattern into its own library, so entity-component-system state and plain-struct state can both plug into the engine the same way.

Everything below is how those nine turned into `src/libs/ecs` and `src/libs/reducer`.

## Planning before code

Same discipline as the replay system: a `docs/PLAN.md` and a `docs/CHECKLIST.md`, written and reviewed before any implementation code, then a `docs/NOTES.md` kept up to date as the plan met reality.
Same fate, too — those files lived under `docs/` only for the duration of the work and are gone by the time you're reading this, which is exactly what makes a post like this worth writing: to keep the parts worth keeping.

Ten commits carried the work, each one buildable and fully green on its own — `docs: plan the ECS library implementation` through `docs: close out the ECS checklist and record implementation notes` — because a checklist item that isn't backed by a passing test at the point it's checked off isn't actually done, just claimed done.

## The core design

### Entity: an integer that can't be recycled

```cpp
enum class Entity : std::uint64_t {};
inline constexpr Entity kNullEntity{0};
```

A scoped enum with no enumerators over `std::uint64_t` — trivially copyable, comparable for free, but not silently interchangeable with an unrelated integer the way a bare `using Entity = std::uint64_t` would be.

The interesting part isn't the type, though — it's what `EntityManager` does with it.
The original plan (before requirement 8 above) had a generation counter: destroy an entity, its index goes onto a free list, a later `create()` reuses the index with a bumped generation, and any stale handle from before the reuse fails an `alive()` check instead of silently aliasing the new entity.
That's the standard answer to a real problem — index reuse plus a plain integer handle is a classic ABA bug waiting to happen.

Then came the actual instruction: don't recycle indices at all, and if the space runs out, log a fatal error and terminate.
That single sentence deleted the entire free-list-and-generation subsystem.
If an index is never reused, there's nothing for a generation counter to guard against — the bug a generation counter exists to prevent can't occur in a scheme that never recycles in the first place.
`EntityManager` ended up simpler *and* the requirement it satisfies is stricter, which isn't the usual direction a simplification goes:

```cpp
Entity EntityManager::create()
{
    if (nextValue > maxEntities)
    {
        logger.log(Level::Fatal, "EntityManager: entity index space exhausted");
        std::exit(EXIT_FAILURE); // GCOVR_EXCL_LINE
    }

    const auto value = nextValue++;
    aliveFlags.push_back(true);
    return Entity{value};
}
```

`maxEntities` defaults to the full range of `std::uint64_t` — exhausting that for real isn't a realistic concern — but it's a constructor parameter specifically so a test can force the fatal path in a handful of calls instead of 2^64.
The test that exercises it constructs a real `Logger` writing to `std::cerr` (not a mock) inside an `EXPECT_DEATH`, so the assertion is on the actual logged message text, not just on the process having died:

```cpp
TEST(EntityManagerDeathTest, ExhaustingIndexSpaceLogsFatalAndTerminates)
{
    EXPECT_DEATH(createUntilExhausted(), "exhausted");
}
```

`Level::Fatal` already existed in `antwika::log::Level` before this — nothing had ever actually acted on it.
This is the first thing in the codebase that does.

### Component: a concept, not a base class

```cpp
template <typename T>
concept Component =
    std::is_trivially_copyable_v<T> && std::is_standard_layout_v<T>;
```

No marker interface, no virtual destructor tax on every piece of gameplay data — just a compile-time constraint, checked wherever `ComponentStorage<T>` or `World::add<T>` gets instantiated.
A `static_assert` in the test suite proves both directions: a plain struct satisfies it, one holding a `std::string` does not.

### Double buffering and phases turn out to be the same mechanism

This is the part the whole design leans on, and it's worth walking through carefully, because the two headline requirements — "no direct mutation, use double buffering" and "systems can be attached to phases" — read like two separate features right up until you try to implement them as two separate features.

Each component type gets its own `ComponentStorage<T>`: a sparse set holding two parallel arrays, `front` and `back`.

```cpp
[[nodiscard]] const T &read(Entity entity) const   { /* returns from front */ }
void write(Entity entity, T value)                 { /* writes into back  */ }
void commit()                                       { front.swap(back); back = front; }
```

`read()` only ever returns `front`.
`write()` only ever touches `back`.
There is no method on `ComponentStorage<T>` — and by extension none on `World`, which is the only thing a system ever gets a reference to — that hands out a mutable reference into `front`.
A system physically cannot affect what another system reads during the same pass; the type system doesn't offer the option.

Now the question a naive reading of "commit once per tick" runs straight into: if `front` only changes once, at the very end of a tick, what's the point of phase ordering?
A "physics" phase could never see what an "input" phase wrote earlier in the same tick, because nothing would have swapped yet.
Phases would exist in name only.

The fix is that the commit point is per *phase*, not per *tick*:

```cpp
void SystemScheduler::run(World &world, antwika::time::Tick tick)
{
    for (const auto &phase : phases)
    {
        for (auto *system : phase.systems) system->update(world, tick);
        world.commit();
    }
}
```

Within one phase, every system reads the identical `front` snapshot taken at the start of that phase — that's the "no direct mutation" guarantee, intact.
Across phases, each phase sees everything every earlier phase in the same tick wrote — that's what makes phase ordering *mean* something instead of being decorative.
An application that never needs ordering just creates one phase and gets plain double buffering with no visible phase concept and no extra cost.

Two tests exist specifically to prove this claim rather than assert it in a comment:

```cpp
TEST(SystemSchedulerTest, SystemsInTheSamePhaseNeverObserveASiblingsWrite)
TEST(SystemSchedulerTest, ALaterPhaseObservesAnEarlierPhasesWrites)
```

The first runs a `SetPositionSystem` and a `RecordPositionSystem` in the same phase and asserts the reader sees the *old* value.
The second puts them in consecutive phases and asserts the reader sees the *new* one.
Between them, that's the entire architectural claim of this design, proven rather than described.

### World: type erasure instead of a polymorphic base

The plan's original sketch had `ComponentStorage<T>` derive from a private, `src/`-only `IComponentPool` base class, so `World` could hold a heterogeneous collection of storages and call `commit()` on all of them without knowing every `T` at any single call site.
That sketch doesn't survive contact with the header layout: `ComponentStorage<T>` is a template, which means it has to be fully defined in a public header — but a base class it derives from can't simultaneously be private under `src/`.
Public template, private base, pick one.

What actually landed instead:

```cpp
std::unordered_map<std::type_index, std::shared_ptr<void>> pools;
std::vector<std::function<void()>> commitCallbacks;
```

The first time a component type is touched, `World` builds a `ComponentStorage<T>`, stores it type-erased behind `shared_ptr<void>`, and captures a `[ptr]{ ptr->commit(); }` lambda alongside it.
`ComponentStorage<T>` ends up with zero dependency on `World` or on any `detail` type at all — no inheritance, nothing to mock, fully testable on its own, which is exactly what `ComponentStorageTest.cpp` and `DoubleBufferingTest.cpp` do.
The type erasure moved from "a base class every storage has to know about" to "a detail only `World` has to know about," which is a smaller thing for a smaller number of places to depend on.

The same header-boundary problem showed up again one layer down.
`World` needs an `EntityManager`, but `EntityManager.hpp` is a private `src/` header, and `World.hpp` is public.
Holding it by value would require the private type to be complete wherever `World.hpp` is included — the exact same public/private conflict as above.
The fix is the standard one: forward-declare, hold it behind `std::unique_ptr`, declare the destructor in the header and define it in `World.cpp` where the type is actually complete.
Every templated `World` method that needs to check aliveness calls the ordinary, non-template `World::alive(Entity) const` instead of touching `entityManager` directly — an out-of-line member function doesn't need its callee's dependencies complete at the call site, only at its own definition site.

Neither of these was a bug.
Both were places where a design sketched in prose looked reasonable right up until the compiler's actual rules for public and private headers had a vote.

### View: a snapshot, not a lazy filter

```cpp
template <Component... Ts>
class View final
{
    explicit View(const ComponentStorage<Ts> *...storages)
    {
        if ((... || (storages == nullptr))) return;
        matching = smallestEntitiesOf(storages...);
        std::erase_if(matching, [&](Entity e) { return !(... && storages->contains(e)); });
    }
    // ...
};
```

`World::view<Position, Velocity>()` builds this by scanning whichever component type currently has the fewest entities and filtering by containment in the rest — computed once, eagerly, at the point it's requested, not as a lazy filtering iterator recomputed on every increment.
That's a deliberate simplification: a system only ever calls `view()` and iterates it within its own `update()`, before anything can commit, so there's no staleness window for a lazy version to protect against — only extra iterator-invariant bookkeeping it would need to get right for no observable benefit.
Order comes from whichever storage is smallest, which is itself insertion-order-stable (component storage uses a stable shift on removal, never swap-and-pop), so the same entity/component history always produces the same iteration order — checked directly by feeding two runs the same churn and diffing the result.

## Generalizing the reducer pattern

The ninth requirement above — consider a shared library for the reducer pattern — turned into `antwika::reducer`, deliberately small and deliberately not a prerequisite for anything above it.

The existing `GameStateReducer` from the replay system already implements `antwika::event::ITimedEventSink`, and that interface doesn't know or care what state it folds events into.
So the actual gap wasn't "invent a generic integration point" — one already existed — it was "stop hand-writing the same mutate-in-place plumbing per application state type."

```cpp
template <typename State>
class IReducer
{
public:
    virtual ~IReducer() = default;
    [[nodiscard]] virtual State reduce(const State &previous, const TimedEvent &event) const = 0;
};

template <typename State>
class ReducerSink final : public ITimedEventSink
{
public:
    ReducerSink(State &state, const IReducer<State> &reducer) : state(state), reducer(reducer) {}
    void handle(const TimedEvent &event) override { state = reducer.reduce(state, event); }
private:
    State &state;
    const IReducer<State> &reducer;
};
```

`IReducer<State>::reduce` is a pure function — previous state and an event in, next state out, no mutation — which is the same discipline `ComponentStorage`'s front/back split enforces for the entity-component-system, applied here to a plain struct instead.
`ReducerSink<State>` is the one generic adapter that makes any `IReducer<State>` pluggable into the engine, replacing what would otherwise be a new mutate-in-place `ITimedEventSink` subclass per application.

The entity-component-system deliberately does not route through this.
`World`'s double-buffer-per-phase lifecycle doesn't fit "return an entirely new state value" — copying every component of every entity every tick just to satisfy a generic interface shaped for small plain structs would be pure waste for no benefit.
`World` keeps its own small `ITimedEventSink` adapter instead.
Both count as "pluggable into the engine as a reducer" in the sense that actually matters: both terminate at the same `ITimedEventSink` seam, without forcing one of them through machinery built for the other's shape.

`antwika_reducer` is header-only — `add_library(antwika_reducer INTERFACE)` — the first interface-only library in this codebase; every other one has real `.cpp` files.
That's not a style choice so much as a consequence of the library containing exactly two templates and nothing else.

`GameStateReducer` was deliberately *not* migrated onto this as part of the same pass.
It's already tested, working code in `apps/game`, and touching it wasn't asked for — the library stands on its own two tests either way.

## Where it ended up

- 2 new libraries: `antwika::ecs` (`Entity`, `EntityManager`, `Component`, `EcsError`, `ComponentStorage<T>`, `World`, `View<Ts...>`, `ISystem`, `SystemScheduler`) and `antwika::reducer` (`IReducer<State>`, `ReducerSink<State>`).
- 49 new tests, none bolted on afterward — `SystemSchedulerTest`'s two same-phase/next-phase tests and `EcsDeterminismTest`'s two-runs-from-scratch comparison are the ones actually carrying the design's core claims.
- 10 commits, each independently green, each a single-line Conventional Commit.
- Two places where the plan as written didn't compile once it met the language's actual rules about public and private headers, and one place where an explicit instruction mid-conversation deleted an entire subsystem in favor of something simpler and stricter at the same time.

If you're extending this: a concrete system only ever needs `World &` and `antwika::time::Tick`; a concrete component only needs to satisfy `Component` (plain, trivially copyable data); and wiring either the entity-component-system or a plain-struct-plus-reducer into the actual tick loop is application code's job, the same way `GameStateReducer` was wired into `Game::bootstrap` for the replay system — `antwika::ecs` and `antwika::reducer` both stay domain-agnostic on purpose.
Left undone, on purpose: a real coverage run through `-DENABLE_COVERAGE=ON` to check whether the fatal-exhaustion line's `GCOVR_EXCL_LINE` marker is actually earning its keep, and whether `REQUIREMENTS.md` — whose error-handling language is currently catchable-error-only — should gain a line about the one deliberately uncatchable exception to that rule.
