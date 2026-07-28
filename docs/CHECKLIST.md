# CHECKLIST: Entity-Component-System (`antwika::ecs`)

Companion to `docs/PLAN.md`. Scaffolding per `REQUIREMENTS.md`'s
"Could have" section — temporary, delete once the ECS lands.

## 0. Scaffold

- [ ] Create `src/libs/ecs/{CMakeLists.txt,include,src,tests}`.
- [ ] `include/antwika/ecs/` directory created, empty.
- [ ] `add_library(antwika_ecs ...)` + `antwika::ecs` alias in
      `src/libs/ecs/CMakeLists.txt`, matching the shape of
      `src/libs/event/CMakeLists.txt` (install rules, export set,
      `WINDOWS_EXPORT_ALL_SYMBOLS ON`).
- [ ] `target_link_libraries(antwika_ecs PUBLIC antwika::time
      antwika::log)` — `time` for `Tick`, `log` for the fatal
      entity-exhaustion path (`docs/PLAN.md` §3.1).
- [ ] `add_subdirectory(ecs)` added to `src/libs/CMakeLists.txt`.
- [ ] `find_package(GTest REQUIRED)` + `antwika_ecs_tests` executable
      scaffolded in `src/libs/ecs/tests/CMakeLists.txt`, registered
      with `gtest_discover_tests`.
- [ ] Project builds with zero `.cpp` files yet (empty lib compiles).

## 1. Entity

- [ ] `Entity.hpp`: `enum class Entity : std::uint64_t {}`,
      `kNullEntity{}` (value `0`); real entities start at `1`.
- [ ] `EntityManager.hpp/.cpp` (private, under `src/`): `create()`,
      `destroy(Entity)`, `alive(Entity) const`.
- [ ] Index allocation is a monotonic counter only — **no recycling,
      no free list, no generation counter**. `destroy()` flips an
      alive flag (e.g. `std::vector<bool>`) and permanently retires
      the index.
- [ ] `EntityManager` constructor takes `antwika::log::ILogger&` and
      an optional `std::uint64_t maxEntities` ceiling (default
      `std::numeric_limits<std::uint64_t>::max()`), overridable so
      tests can force exhaustion in a handful of calls.
- [ ] `create()` past the ceiling: log `Level::Fatal` via the injected
      logger, then `std::exit(EXIT_FAILURE)` — not an `EcsError`, not
      a normal return.
- [ ] `EntityManagerTest.cpp`: `create()` returns strictly increasing
      values starting at `1`; `destroy()` then `alive()` is false and
      the index is never handed out again by a later `create()`;
      double-destroy is rejected (`EcsError`).
- [ ] `EntityManagerTest.cpp` (death test): construct with a tiny
      `maxEntities` and a mock `ILogger`; `EXPECT_DEATH` on the
      `create()` call that exceeds it; assert the fatal log happened
      first (via the mock, in a non-death-test call, or via captured
      stderr in the death-test matcher).

## 2. Component concept and error type

- [ ] `Component.hpp`: `concept Component` requiring
      `std::is_trivially_copyable_v` + `std::is_standard_layout_v`.
- [ ] `EcsError.hpp`: one exception type, constructed with a reason
      (missing entity, dead entity, missing component, etc.), mirrors
      `ReplayFormatError`'s "one specific, catchable type" shape.
      Entity-ID exhaustion (§1) deliberately does **not** use this —
      it terminates instead, see `docs/PLAN.md` §3.1/§3.8.
- [ ] A `static_assert(Component<...>)` or concept-based
      compile-fail test exists for a type that is *not* trivially
      copyable (e.g. one holding a `std::string`), proving the
      constraint actually rejects non-POD components.

## 3. Component storage (double buffering)

- [ ] `ComponentStorage.hpp` (header-only template): sparse set with
      parallel `front`/`back` value arrays and a dense `Entity` array.
- [ ] `insert(Entity, T)`, `remove(Entity)` (queued/staged, not
      immediate — see `commit()` below), `contains(Entity) const`.
- [ ] `read(Entity) const` returns from `front` only.
- [ ] `write(Entity, T)` writes into `back` only — no API path
      exists to get a mutable reference into `front`.
- [ ] `entities() const` exposes the dense array (front) in stable,
      insertion-preserving order.
- [ ] `commit()`: swap `front`/`back`, then reseed the new `back` as
      a copy of the new `front`; apply any queued insert/remove.
- [ ] Removal is implemented as a stable erase (shift), not
      swap-and-pop — iteration order must not depend on unrelated
      removal timing.
- [ ] `ComponentStorageTest.cpp`: insert/read/contains/remove behave
      as expected; a `write()` is invisible to `read()` until
      `commit()`; order of `entities()` survives interleaved
      insert/remove sequences deterministically (same sequence twice
      -> same order).
- [ ] `DoubleBufferingTest.cpp` (storage-level): two "writes" to the
      same entity before a `commit()` — last write wins, deterministic
      by call order, never by container iteration order.

## 4. World

- [ ] `IComponentPool.hpp` (private, under `src/`): type-erased base
      with `commit()`, used so `World` can iterate heterogeneous
      `ComponentStorage<T>` instances without knowing every `T`.
- [ ] `World.hpp/.cpp`: owns an `EntityManager` and a
      type-indexed map of `IComponentPool` instances (keyed by
      `std::type_index`, lookup only — never iterated in an
      order-sensitive way).
- [ ] `World` constructor takes `antwika::log::ILogger&` and forwards
      it (plus any `maxEntities` override) to its `EntityManager`.
- [ ] `create()`, `destroy(Entity)` (queued), `alive(Entity) const`.
- [ ] `add<T>(Entity, T)` (queued), `remove<T>(Entity)` (queued),
      `has<T>(Entity) const`.
- [ ] `get<T>(Entity) const` (front) / `set<T>(Entity, T)` (back);
      both throw `EcsError` for a dead entity or absent component.
- [ ] `commit()`: applies all queued structural changes (create being
      already immediate, destroy/add/remove applied here), then calls
      `commit()` on every registered pool.
- [ ] `WorldTest.cpp`: full lifecycle (create, add components, read,
      write, commit, read again, destroy, verify gone); a destroyed
      entity's stale handle raises `EcsError` on subsequent access.

## 5. Views

- [ ] `View.hpp` (header-only template `View<Ts...>`): iterates
      entities that have every `T` in `Ts...`, built by scanning the
      smallest storage's dense array and filtering with `has<T>()`.
- [ ] `World::view<Ts...>()` returns a `View<Ts...>` over `front`.
- [ ] Supports range-`for` (begin/end), read-only.
- [ ] `ViewTest.cpp`: correct intersection over 2+ component types;
      empty view when no entity has every required component;
      iteration order is deterministic across repeated runs with the
      same entity/component churn.

## 6. Systems and phases

- [ ] `ISystem.hpp`: `update(World &, antwika::time::Tick)`.
- [ ] `Phase.hpp`: `using PhaseId = std::uint32_t;`.
- [ ] `SystemScheduler.hpp/.cpp`: `createPhase(name)` (order of
      creation call is execution order), `addSystem(phase, ISystem&)`
      (registration order within a phase is execution order), `run`.
- [ ] `run(World&, Tick)`: for each phase in creation order, run its
      systems in registration order, then call `world.commit()` once
      for that phase before moving to the next phase.
- [ ] `tests/mocks/include/antwika/ecs/mocks/MockSystem.hpp` created
      and consumed by at least one `.cpp` test.
- [ ] `SystemSchedulerTest.cpp`: systems execute in phase-creation
      order, then registration order; a later phase observes an
      earlier phase's writes in the same tick; a system does not
      observe a same-phase sibling's write.
- [ ] `EcsDeterminismTest.cpp`: fixed systems/phases/starting state,
      run N ticks twice from scratch, assert identical final
      component values and identical entity iteration order — same
      shape as `src/libs/replay/tests/ReplayDeterminismTest.cpp`.

## 7. Cross-cutting / hygiene

- [ ] No line in `src/libs/ecs/**/*.{hpp,cpp}` exceeds 80 characters
      (`scripts/check_line_length.py` covers `src/**` already).
- [ ] Doxygen `@brief`/`@param`/`@return` on every public class and
      method under `include/antwika/ecs/`.
- [ ] No `std::unordered_map`/`unordered_set` (or anything else whose
      iteration order isn't a documented, stable invariant) used
      anywhere iteration order could leak into system behavior.
      `World`'s type-indexed pool map may use one internally, since
      it's looked up by type, never iterated in a way that affects
      simulation output — call this out at the point of use.
- [ ] No RNG/PRNG anywhere in the library.
- [ ] `-Wall -Wextra -Wpedantic -Wsuggest-override -Werror` clean on
      GNU and LLVM toolchains.
- [ ] `README.md`'s project-structure listing gains `ecs/` under
      `libs/`.
- [ ] Coverage: GNU/LLVM builds pass with the new tests included; any
      `GCOVR_EXCL_LINE` is justified by a comment, added only after
      real coverage gaps are confirmed. The `std::exit(EXIT_FAILURE)`
      line in the exhaustion path is a likely, legitimate candidate
      if the death test can't make gcovr see it as covered.
- [ ] Once landed, consider whether `REQUIREMENTS.md` should gain a
      line documenting the fatal/terminating exhaustion behavior
      (currently its error-handling language is catchable-error-only)
      — a deliberate, motivated adjustment, not an oversight
      (`docs/PLAN.md` §9).

## 8. Related, non-blocking: `antwika::reducer`

Separate library (`docs/PLAN.md` §6), generalizing the existing
hand-rolled reducer pattern so ECS-style and plain-struct state both
plug into the engine through the same `ITimedEventSink` seam. Neither
this library nor the ECS above depends on the other — sequence
freely.

- [ ] `src/libs/reducer/{CMakeLists.txt,include,src,tests}` scaffold.
      `add_library(antwika_reducer INTERFACE)` — header-only,
      confirm this is a deliberate, documented first for the repo
      (every other library so far has real `.cpp` files).
- [ ] `target_link_libraries(antwika_reducer INTERFACE
      antwika::event)`.
- [ ] `include/antwika/reducer/IReducer.hpp`: `template <typename
      State> class IReducer` with a pure
      `reduce(const State&, const TimedEvent&) const -> State`.
- [ ] `include/antwika/reducer/ReducerSink.hpp`: `template <typename
      State> class ReducerSink final : public ITimedEventSink`,
      applying `IReducer<State>::reduce` and storing the result back
      into a referenced `State`.
- [ ] `ReducerSinkTest.cpp`: a trivial `IReducer<int>` (or similar)
      proves `ReducerSink` calls `reduce` and updates the referenced
      state on `handle()`.
- [ ] `tests/mocks/include/antwika/reducer/mocks/MockReducer.hpp`
      (a `GMock`-based `IReducer<State>` double) created and consumed
      by at least one `.cpp` test.
- [ ] Optional demonstration, not required to land the library: migrate
      `GameStateReducer` in `src/apps/game` to a pure
      `IReducer<GameState>` implementation plugged into a
      `ReducerSink<GameState>`, replacing the current mutate-in-place
      class. Update `GameStateReducerTest.cpp` accordingly (should get
      simpler — pure function in, value out, no fixture mutation).
- [ ] If the migration happens, `Game::bootstrap` in `Game.cpp`
      updated to construct `ReducerSink<GameState>` instead of
      `GameStateReducer` directly.

## 9. Done means

- [ ] All of §0–§7 checked (§8 is optional/separate, see above).
- [ ] `docs/PLAN.md` and `docs/CHECKLIST.md` deleted (temporary
      scaffolding, per `REQUIREMENTS.md`).
- [ ] Optional: a `blog/` post if the double-buffering-via-phases
      design decision (and/or the reducer generalization) is worth
      writing up, matching the precedent set by the replay-system
      post.
