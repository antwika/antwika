# NOTES: implementing antwika::ecs

Running log of decisions made while implementing `docs/PLAN.md`, kept
mainly for whoever writes the eventual `blog/` post (see
`REQUIREMENTS.md`'s "Could have": these `docs/` files are temporary).

## Deviations from the original file layout

- **No `IComponentPool.hpp` private virtual base.** The plan's original
  sketch had `ComponentStorage<T>` derive from a polymorphic
  `detail::IComponentPool` so `World` could hold a heterogeneous
  collection and call `commit()` on all of them. That doesn't work
  cleanly: `ComponentStorage<T>` is a public template header
  (`include/antwika/ecs/ComponentStorage.hpp`), so its base class would
  also have to be public, defeating the point of keeping it private
  under `src/`.

  Landed on type erasure instead: `World` keeps
  `std::unordered_map<std::type_index, std::shared_ptr<void>> pools`,
  plus two parallel `std::vector<std::function<...>>` lists
  (`commitCallbacks`, `removeFromAllPools`) populated the first time a
  component type is touched. `ComponentStorage<T>` stays a completely
  standalone template — no inheritance, no dependency on `World` or
  any `detail` type, more reusable and easier to unit-test in
  isolation. This is arguably more in the spirit of "modern C++
  features" than the original virtual-base sketch.

- **`World` holds its `EntityManager` via pimpl.** Same root issue:
  `EntityManager` is a private `src/` header, but `World.hpp` is
  public and needs an `EntityManager` data member. Made
  `detail::EntityManager` forward-declared in `World.hpp`, held as
  `std::unique_ptr<detail::EntityManager>`, with `World`'s destructor
  declared in the header and defined in `World.cpp` (where the type is
  complete) — the standard pattern for this. Templated `World` methods
  never touch `entityManager` directly; they all go through the
  non-template `World::alive(Entity) const` (and a private
  `World::retire(Entity)` for the destroy path), which are ordinary
  out-of-line member functions and so don't need the type complete at
  the template's definition point.

- **Structural-change queuing lives entirely in `World`, not in
  `ComponentStorage<T>`.** The plan's wording ("insert/remove queued,
  not immediate") reads as if `ComponentStorage` itself queues.
  Simpler in practice: `ComponentStorage<T>::insert`/`remove` are
  plain, immediate operations; `World::add<T>`/`remove<T>`/`destroy`
  are the ones that defer, by pushing a `std::function<void()>` onto
  `World::pendingOps`, only replayed inside `World::commit()`. This
  keeps `ComponentStorage<T>` simple and independently testable
  (`ComponentStorageTest.cpp` exercises it with zero queuing
  concepts), and keeps "what's deferred and why" in one place.

## Confirmed by testing, not just asserted

- `EntityManagerTest.ExhaustingIndexSpaceLogsFatalAndTerminates` uses
  `EXPECT_DEATH` with a real `Logger`/`StreamAppender(std::cerr)` (not
  a mock) so the death test can assert on the actual logged message
  text, not just that the process died. `EntityManager`'s `maxEntities`
  constructor parameter exists specifically to make this reachable in
  a handful of calls instead of 2^64.
- `SystemSchedulerTest` has two tests that directly exercise the
  double-buffering-via-phases design from `PLAN.md` §3.4: one proves a
  system never observes a same-phase sibling's write, the other proves
  a later phase observes an earlier phase's write from the same tick.
  These two tests are the actual proof the central design idea works,
  more so than the prose in the plan.
- `EcsDeterminismTest` runs a small multi-tick simulation twice from
  scratch and diffs the results, plus a second test that diffs
  `view()` iteration order across two runs of identical entity/
  component churn — same shape as
  `src/libs/replay/tests/ReplayDeterminismTest.cpp`.

## Coverage

Not yet run through a `-DENABLE_COVERAGE=ON` build as part of this
work. The `std::exit(EXIT_FAILURE)` line in `EntityManager::create()`
is marked `GCOVR_EXCL_LINE` pre-emptively (`CHECKLIST.md` flagged this
as "a likely, legitimate candidate") since death-test coverage from a
forked child process is not reliably attributed back to the parent's
`.gcda` by gcovr. Worth a real coverage run before considering this
done, to check whether that line actually needs the exclusion or gets
picked up anyway.

## What's landed vs. what's still open

Core library (`docs/CHECKLIST.md` §0–§7): done — Entity, EntityManager
(no reuse, fatal exhaustion), Component concept, EcsError,
ComponentStorage<T>, World, View, ISystem, SystemScheduler with
phases, determinism tests, README updated.

Not done, deliberately out of scope for this pass:
- No app-side integration (`EcsTickSystem` wiring the ECS into
  `src/apps/game`'s tick loop via `ITimedEventSink`) — `PLAN.md` §5
  always described this as a separate decision.
- `antwika::reducer` (§6/§8) — separate, non-blocking library.
