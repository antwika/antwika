# CHECKLIST: Entity-Component-System (`antwika::ecs`)

Companion to `docs/PLAN.md`. Scaffolding per `REQUIREMENTS.md`'s
"Could have" section — temporary, delete once the ECS lands.

## 0. Scaffold

- [x] Create `src/libs/ecs/{CMakeLists.txt,include,src,tests}`.
- [x] `include/antwika/ecs/` directory created, empty.
- [x] `add_library(antwika_ecs ...)` + `antwika::ecs` alias in
      `src/libs/ecs/CMakeLists.txt`, matching the shape of
      `src/libs/event/CMakeLists.txt` (install rules, export set,
      `WINDOWS_EXPORT_ALL_SYMBOLS ON`).
- [x] `target_link_libraries(antwika_ecs PUBLIC antwika::time
      antwika::log)` — `time` for `Tick`, `log` for the fatal
      entity-exhaustion path (`docs/PLAN.md` §3.1).
- [x] `add_subdirectory(ecs)` added to `src/libs/CMakeLists.txt`.
- [x] `find_package(GTest REQUIRED)` + `antwika_ecs_tests` executable
      scaffolded in `src/libs/ecs/tests/CMakeLists.txt`, registered
      with `gtest_discover_tests`.
- [x] ~~Project builds with zero `.cpp` files yet~~ — landed scaffold
      and `EntityManager` together instead; an empty `add_library`
      with no sources isn't meaningful, see `docs/NOTES.md`.

## 1. Entity

- [x] `Entity.hpp`: `enum class Entity : std::uint64_t {}`,
      `kNullEntity{}` (value `0`); real entities start at `1`.
- [x] `EntityManager.hpp/.cpp` (private, under `src/`): `create()`,
      `destroy(Entity)`, `alive(Entity) const`.
- [x] Index allocation is a monotonic counter only — **no recycling,
      no free list, no generation counter**. `destroy()` flips an
      alive flag (e.g. `std::vector<bool>`) and permanently retires
      the index.
- [x] `EntityManager` constructor takes `antwika::log::ILogger&` and
      an optional `std::uint64_t maxEntities` ceiling (default
      `std::numeric_limits<std::uint64_t>::max()`), overridable so
      tests can force exhaustion in a handful of calls.
- [x] `create()` past the ceiling: log `Level::Fatal` via the injected
      logger, then `std::exit(EXIT_FAILURE)` — not an `EcsError`, not
      a normal return.
- [x] `EntityManagerTest.cpp`: `create()` returns strictly increasing
      values starting at `1`; `destroy()` then `alive()` is false and
      the index is never handed out again by a later `create()`;
      double-destroy is rejected (`EcsError`).
- [x] `EntityManagerTest.cpp` (death test): construct with a tiny
      `maxEntities`; `EXPECT_DEATH` on the `create()` call that
      exceeds it, asserting on the fatal message captured from a real
      `Logger` writing to `std::cerr` (not a mock — see
      `docs/NOTES.md`).

## 2. Component concept and error type

- [x] `Component.hpp`: `concept Component` requiring
      `std::is_trivially_copyable_v` + `std::is_standard_layout_v`.
- [x] `EcsError.hpp`: one exception type, constructed with a reason
      (missing entity, dead entity, missing component, etc.), mirrors
      `ReplayFormatError`'s "one specific, catchable type" shape.
      Entity-ID exhaustion (§1) deliberately does **not** use this —
      it terminates instead, see `docs/PLAN.md` §3.1/§3.8.
- [x] A `static_assert(Component<...>)` compile-fail check exists for
      a type that is *not* trivially copyable (`ComponentTest.cpp`),
      proving the constraint actually rejects non-POD components.

## 3. Component storage (double buffering)

- [x] `ComponentStorage.hpp` (header-only template): sparse set with
      parallel `front`/`back` value arrays and a dense `Entity` array.
- [x] `insert(Entity, T)`, `remove(Entity)`, `contains(Entity) const`
      — immediate at the storage level; it's `World` that defers
      structural changes, not `ComponentStorage` itself, see
      `docs/NOTES.md`.
- [x] `read(Entity) const` returns from `front` only.
- [x] `write(Entity, T)` writes into `back` only — no API path
      exists to get a mutable reference into `front`.
- [x] `entities() const` exposes the dense array (front) in stable,
      insertion-preserving order.
- [x] `commit()`: swap `front`/`back`, then reseed the new `back` as
      a copy of the new `front`.
- [x] Removal is implemented as a stable erase (shift), not
      swap-and-pop — iteration order must not depend on unrelated
      removal timing.
- [x] `ComponentStorageTest.cpp`: insert/read/contains/remove behave
      as expected; a `write()` is invisible to `read()` until
      `commit()`; order of `entities()` survives interleaved
      insert/remove sequences deterministically (same sequence twice
      -> same order).
- [x] `DoubleBufferingTest.cpp` (storage-level): two "writes" to the
      same entity before a `commit()` — last write wins, deterministic
      by call order, never by container iteration order.

## 4. World

- [x] ~~`IComponentPool.hpp` private virtual base~~ — doesn't work:
      `ComponentStorage<T>` is a public template header, so a base
      class it derives from can't be private under `src/`. Used type
      erasure instead (`std::type_index` -> `std::shared_ptr<void>`
      plus captured `std::function` callbacks). See `docs/NOTES.md`.
- [x] `World.hpp/.cpp`: owns an `EntityManager` (via pimpl — it's a
      private type too, see `docs/NOTES.md`) and a type-indexed map of
      component pools, looked up by `std::type_index`, never iterated
      in an order-sensitive way.
- [x] `World` constructor takes `antwika::log::ILogger&` and forwards
      it (plus any `maxEntities` override) to its `EntityManager`.
- [x] `create()`, `destroy(Entity)` (queued), `alive(Entity) const`.
- [x] `add<T>(Entity, T)` (queued), `remove<T>(Entity)` (queued),
      `has<T>(Entity) const`.
- [x] `get<T>(Entity) const` (front) / `set<T>(Entity, T)` (back);
      both throw `EcsError` for a dead entity or absent component.
- [x] `commit()`: applies all queued structural changes (create being
      already immediate, destroy/add/remove applied here), then calls
      `commit()` on every registered pool.
- [x] `WorldTest.cpp`: full lifecycle (create, add components, read,
      write, commit, read again, destroy, verify gone); a destroyed
      entity's stale handle raises `EcsError` on subsequent access.

## 5. Views

- [x] `View.hpp` (header-only template `View<Ts...>`): a snapshot,
      computed once at construction by scanning the smallest storage's
      dense array and filtering with `contains<T>()`, not a lazy
      filtering iterator — simpler to get correct, see
      `docs/NOTES.md`.
- [x] `World::view<Ts...>()` returns a `View<Ts...>` over `front`.
- [x] Supports range-`for` (begin/end), read-only.
- [x] `ViewTest.cpp`: correct intersection over 2+ component types;
      empty view when no entity has every required component;
      iteration order is deterministic across repeated runs with the
      same entity/component churn.

## 6. Systems and phases

- [x] `ISystem.hpp`: `update(World &, antwika::time::Tick)`.
- [x] `Phase.hpp`: `using PhaseId = std::uint32_t;`.
- [x] `SystemScheduler.hpp/.cpp`: `createPhase(name)` (order of
      creation call is execution order), `addSystem(phase, ISystem&)`
      (registration order within a phase is execution order), `run`.
- [x] `run(World&, Tick)`: for each phase in creation order, run its
      systems in registration order, then call `world.commit()` once
      for that phase before moving to the next phase.
- [x] `tests/mocks/include/antwika/ecs/mocks/MockSystem.hpp` created
      and consumed by at least one `.cpp` test.
- [x] `SystemSchedulerTest.cpp`: systems execute in phase-creation
      order, then registration order; a later phase observes an
      earlier phase's writes in the same tick; a system does not
      observe a same-phase sibling's write.
- [x] `EcsDeterminismTest.cpp`: fixed systems/phases/starting state,
      run N ticks twice from scratch, assert identical final
      component values and identical entity iteration order — same
      shape as `src/libs/replay/tests/ReplayDeterminismTest.cpp`.

## 7. Cross-cutting / hygiene

- [x] No line in `src/libs/ecs/**/*.{hpp,cpp}` exceeds 80 characters
      (`scripts/check_line_length.py` covers `src/**` already).
- [x] Doxygen `@brief`/`@param`/`@return` on every public class and
      method under `include/antwika/ecs/`.
- [x] No `std::unordered_map`/`unordered_set` (or anything else whose
      iteration order isn't a documented, stable invariant) used
      anywhere iteration order could leak into system behavior.
      `World`'s type-indexed pool map does use one internally, since
      it's looked up by type, never iterated in a way that affects
      simulation output.
- [x] No RNG/PRNG anywhere in the library.
- [x] `-Wall -Wextra -Wpedantic -Wsuggest-override -Werror` clean
      (verified on this devcontainer's toolchain; GNU/LLVM CI matrix
      not separately run as part of this pass).
- [x] `README.md`'s project-structure listing gains `ecs/` under
      `libs/`.
- [ ] Coverage: not yet run through a `-DENABLE_COVERAGE=ON` build.
      The `std::exit(EXIT_FAILURE)` line in the exhaustion path is
      pre-emptively marked `GCOVR_EXCL_LINE` with a rationale comment;
      worth a real coverage run to confirm that's actually needed
      before calling this fully done — see `docs/NOTES.md`.
- [ ] Once landed, consider whether `REQUIREMENTS.md` should gain a
      line documenting the fatal/terminating exhaustion behavior
      (currently its error-handling language is catchable-error-only)
      — a deliberate, motivated adjustment, not an oversight
      (`docs/PLAN.md` §9). Left for the user to decide.

## 8. Related, non-blocking: `antwika::reducer`

Separate library (`docs/PLAN.md` §6), generalizing the existing
hand-rolled reducer pattern so ECS-style and plain-struct state both
plug into the engine through the same `ITimedEventSink` seam. Neither
this library nor the ECS above depends on the other — sequence
freely.

- [x] `src/libs/reducer/{CMakeLists.txt,include,src,tests}` scaffold.
      `add_library(antwika_reducer INTERFACE)` — header-only, the
      first `INTERFACE`-only library in the repo (every other library
      has real `.cpp` files); no `src/` directory since there's
      nothing non-template to put there.
- [x] `target_link_libraries(antwika_reducer INTERFACE
      antwika::event)`.
- [x] `include/antwika/reducer/IReducer.hpp`: `template <typename
      State> class IReducer` with a pure
      `reduce(const State&, const TimedEvent&) const -> State`.
- [x] `include/antwika/reducer/ReducerSink.hpp`: `template <typename
      State> class ReducerSink final : public ITimedEventSink`,
      applying `IReducer<State>::reduce` and storing the result back
      into a referenced `State`.
- [x] `ReducerSinkTest.cpp`: a trivial `IReducer<int>` (or similar)
      proves `ReducerSink` calls `reduce` and updates the referenced
      state on `handle()`.
- [x] `tests/mocks/include/antwika/reducer/mocks/MockReducer.hpp`
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

- [x] §0–§6 checked; §7 checked except the coverage run and the
      `REQUIREMENTS.md` question, both left open on purpose (see
      those items).
- [x] §8 (`antwika::reducer`) landed, except the optional
      `GameStateReducer` migration demonstration — deliberately
      skipped so this pass doesn't touch already-tested, working
      `src/apps/game` code without being asked.
- [ ] `docs/PLAN.md`, `docs/CHECKLIST.md` and `docs/NOTES.md` deleted
      (temporary scaffolding, per `REQUIREMENTS.md`) — left for the
      user, same as the two open items above. The precedent
      (`47224a5`, the replay system) replaced these with a `blog/`
      post first; happy to do the same here on request.
- [ ] Optional: a `blog/` post if the double-buffering-via-phases
      design decision (and/or the reducer generalization) is worth
      writing up, matching the precedent set by the replay-system
      post.
