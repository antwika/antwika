# CHECKLIST: Job scheduler (`antwika::scheduler`) and `apps/task-worker`

Companion to `PLAN_SCHEDULER.md` — read that first, section numbers below
(e.g. "§3.5") refer to it. Step-by-step, in dependency order; later items
generally assume earlier ones are done.

**Revision note:** job dependency/DAG chaining (§3.7 of the plan) was
added after this checklist's first pass — item 4 and item 6 below are
new, and items 3/5/9/12/13/14/16/18 were updated to match. `Priority`'s
representation (item 1) is no longer an open question.

## 0. `antwika::scheduler` scaffold

- [ ] Create `src/libs/scheduler/{CMakeLists.txt,include,src,tests}`.
- [ ] `include/antwika/scheduler/` directory created, empty.
- [ ] `add_library(antwika_scheduler ...)` + `antwika::scheduler` alias in
      `src/libs/scheduler/CMakeLists.txt`, matching the shape of
      `src/libs/ecs/CMakeLists.txt` (install rules, export set,
      `WINDOWS_EXPORT_ALL_SYMBOLS ON`).
- [ ] `target_link_libraries(antwika_scheduler PUBLIC antwika::time)` —
      just `time`, for `Tick` (§3.9 — no `log` dependency, unlike `ecs`).
- [ ] `add_subdirectory(scheduler)` added to `src/libs/CMakeLists.txt`.
- [ ] `find_package(GTest REQUIRED)` + `antwika_scheduler_tests`
      executable scaffolded in `src/libs/scheduler/tests/CMakeLists.txt`,
      registered with `gtest_discover_tests`.
- [ ] Project builds with zero `.cpp` files yet (empty lib compiles).

## 1. `JobId` and `Priority`

- [ ] `JobId.hpp`: `enum class JobId : std::uint64_t {}`,
      `kInvalidJobId{0}`; real jobs get `1` and up.
- [ ] `Priority.hpp`: scoped-enum-over-`std::uint8_t`, decided per §3.1
      (an `Entity`-style wrapper, not a bare alias) — with
      `kLowPriority`/`kNormalPriority`/`kHighPriority`/`kCriticalPriority`
      constants.
- [ ] Both `JobId` and `Priority` get a `rawValue()` free function
      mirroring `antwika::ecs::rawValue(Entity)`.

## 2. `IJob`

- [ ] `IJob.hpp`: `virtual ~IJob() = default;` +
      `virtual void execute(antwika::time::Tick tick) = 0;` — no other
      members (§3.2).

## 3. `Scheduler` core (schedule / run / pending / empty)

- [ ] `Scheduler.hpp/.cpp`: `schedule(IJob &, Priority,
      std::vector<JobId> dependsOn = {}) -> JobId`,
      `run(Tick, std::size_t budget) -> std::vector<JobId>`,
      `pending() const -> std::size_t`, `empty() const -> bool` (§3.3).
- [ ] `schedule()` stores a non-owning `IJob *`; `JobId`s are handed out
      by a monotonic counter starting at `1`, never reused.
- [ ] `pending()` counts jobs that are ready **or** blocked (§3.3) — not
      just the ready queue's size.
- [ ] `SchedulerError.hpp`: one exception type, `std::runtime_error`-
      derived, mirroring `EcsError`'s exact shape (§3.3).
- [ ] Ready-queue ordering is deterministic (a plain `std::vector`,
      sorted-on-insert or heap-maintained — §3.4/§9), comparator exactly
      `priority desc, then id asc` (§3.5) — **no**
      `std::unordered_map`/`unordered_set` anywhere in the pending-job
      or dependency-tracking path (see item 4 for the latter).
- [ ] `run(tick, budget)` captures `epoch = nextId` at entry; only
      entries with `id < epoch` are ever candidates for this call, no
      matter whether they were ready at entry or became ready mid-call
      via a dependency completing (§3.6) — implemented (with a test
      proving it, item 5/6), not just documented.
- [ ] `run()` calls each selected job's `execute(tick)` in priority
      order, marks it completed, resolves any now-unblocked dependents
      into the ready queue (item 4), and returns the executed `JobId`s
      in the order they ran.
- [ ] `budget == 0` returns an empty vector and mutates nothing.
- [ ] `budget >= ` (everything ready and eligible this call) runs all of
      it and returns a vector of that size — no error, no clamping
      surprise.

## 4. `Scheduler` core: job dependencies (DAG chaining, §3.7)

- [ ] `schedule()`'s `dependsOn` is de-duplicated before use, so listing
      the same `JobId` twice behaves identically to listing it once.
- [ ] Each entry in `dependsOn` is validated as a `JobId` this
      `Scheduler` actually issued (i.e. `0 < rawValue(id) < nextId` at
      call time); an unknown one throws `SchedulerError` — nothing is
      partially applied on this path (either the whole `schedule()` call
      succeeds, or it throws before mutating any state).
- [ ] Already-`completed` dependencies don't count toward the new job's
      unmet total; a job with zero unmet dependencies (no `dependsOn`,
      or every entry already completed) goes straight to the ready
      queue, ordered as usual.
- [ ] A job with one or more unmet dependencies is tracked as blocked
      (invisible to `run()`'s priority ordering) and, for each unmet
      dependency, recorded as that dependency's dependent — via a
      `JobId`-indexed structure (plain growable `std::vector`, not a
      hash map — §3.4), not a scan.
- [ ] Completing a job (in `run()`) looks up its recorded dependents,
      decrements each one's unmet count, and moves any that reach `0`
      into the ready queue, in priority order — same insertion logic as
      any other ready-queue entry (item 3), no special case.
- [ ] A job moved to ready *during* a `run()` call this way is eligible
      for that same call's remaining budget (the same-tick cascade,
      §3.7) — bounded only by budget and by `epoch` (item 3), same as
      every other candidate.
- [ ] No cycle-detection code exists anywhere in this path — confirm by
      design review, not by writing (and then deleting) a detection
      algorithm: the acyclicity argument in §3.7 is structural, so there
      is nothing to implement here beyond the validation bullet above.

## 5. `Scheduler` unit tests (`SchedulerTest.cpp`)

- [ ] `schedule()` returns strictly increasing `JobId`s starting at `1`.
- [ ] `pending()`/`empty()` reflect the queue accurately through
      schedule/run cycles, including while jobs are blocked.
- [ ] Jobs scheduled in a deliberately "wrong" order (e.g. low priority
      first, high priority last) still `run()` highest-priority-first;
      equal-priority jobs run in the order they were `schedule()`d
      (§3.5) — assert the exact order, not just "high ran before low".
- [ ] `run()` never executes more than `budget` jobs in one call, even
      when more are pending.
- [ ] A job's `execute()` that calls `schedule()` (with no dependencies)
      on the same `Scheduler` does not get included in the `run()` call
      it was scheduled during; it's picked up by the following `run()`
      call (§3.6) — give it a dedicated test, not a side-assertion in
      another test.
- [ ] `budget == 0` test: no job runs, `pending()` unchanged, returned
      vector is empty.
- [ ] Starvation demo test (§3.6): schedule a `kLowPriority` job, then
      repeatedly schedule+run `kCriticalPriority` jobs with a budget
      that never lets the low-priority one through; assert it's still
      `pending()` at the end — proves the documented limitation is the
      actual behavior, not aspirational prose.
- [ ] `tests/mocks/include/antwika/scheduler/mocks/MockJob.hpp` (GMock
      `IJob` double) created and consumed by at least one `.cpp` test.

## 6. `SchedulerDependencyTest.cpp` (§3.7)

- [ ] A job with one unmet dependency doesn't run until that dependency
      has (assert it's absent from `run()`'s return value beforehand,
      present after the dependency completes).
- [ ] A diamond (`D` depends on `B` and `C`, both depending on `A`)
      resolves correctly regardless of `B`/`C`'s relative priority — `D`
      only becomes ready once *both* have run.
- [ ] Same-tick cascade: a pre-existing, fully-ready-except-for-ordering
      chain (e.g. `A` -> `B` -> `C`, all `schedule()`d before the `run()`
      call under test) drains across all three hops within one `run()`
      call when budget allows — asserted via `run()`'s single return
      value containing all three, in order, not via three separate
      `run()` calls.
- [ ] Epoch rule holds under dependencies too: a job newly `schedule()`d
      *during* another job's `execute()`, even with a `dependsOn` that
      resolves within that same call, is still excluded from that call
      (picked up next call instead) — proves item 3's epoch rule and
      item 4's cascade rule compose correctly rather than one silently
      overriding the other.
- [ ] An unknown/foreign `JobId` in `dependsOn` throws `SchedulerError`,
      and `schedule()`'s state is unchanged after the throw (no
      half-registered job).
- [ ] The same `JobId` listed twice in `dependsOn` behaves identically
      to listing it once (single unmet-count contribution).
- [ ] A dependency that never resolves (e.g. starved per item 5's
      starvation test) leaves its dependent `pending()` forever, with no
      crash, timeout, or special-cased behavior.

## 7. `SchedulerDeterminismTest.cpp`

- [ ] Fixed set of jobs (mixed priorities, known submission order, and
      at least one dependency edge), fixed sequence of
      `run(tick, budget)` calls across several ticks, executed twice
      from scratch (two fresh `Scheduler` instances, same inputs).
      Assert the two runs' full sequence of `run()` return values is
      identical — same shape as
      `src/libs/ecs/tests/EcsDeterminismTest.cpp` and
      `src/libs/replay/tests/ReplayDeterminismTest.cpp`.

## 8. `antwika::scheduler` hygiene

- [ ] No line in `src/libs/scheduler/**/*.{hpp,cpp}` exceeds 80
      characters (`scripts/check_line_length.py` covers `src/**`
      already).
- [ ] Doxygen `@brief`/`@param`/`@return` on every public class/method
      under `include/antwika/scheduler/`, including `SchedulerError`.
- [ ] `-Wall -Wextra -Wpedantic -Wsuggest-override -Werror` clean on GNU
      and LLVM toolchains.
- [ ] Coverage: GNU/LLVM builds pass with the new tests included; no new
      `GCOVR_EXCL_LINE` expected for this library (no fatal/terminating
      path like `EntityManager`'s) — if one turns out to be needed,
      follow `docs/confirming-unreachable-branches.md`'s procedure.
- [ ] `README.md`'s project-structure listing gains `scheduler/` under
      `libs/`.

## 9. `apps/task-worker` scaffold

- [ ] Create
      `src/apps/task-worker/{CMakeLists.txt,include,src,tests}`.
- [ ] `add_executable(antwika_task_worker ...)` in
      `src/apps/task-worker/CMakeLists.txt`, matching
      `src/apps/life/CMakeLists.txt`'s shape (include dirs, link
      libraries, the MinGW DLL-copy block, install rule).
- [ ] `target_link_libraries(antwika_task_worker PRIVATE antwika::engine
      antwika::replay antwika::time antwika::ecs antwika::event
      antwika::scheduler antwika::log)` (superset of `apps/life`'s link
      list, plus `antwika::scheduler`).
- [ ] `add_subdirectory(task-worker)` added to `src/apps/CMakeLists.txt`.
- [ ] `find_package(GTest REQUIRED)` +
      `antwika_task_worker_tests` executable scaffolded in
      `src/apps/task-worker/tests/CMakeLists.txt`.
- [ ] Everything under `src/apps/task-worker` uses the
      `antwika::task_worker` C++ namespace (§4.6 — hyphenated directory,
      underscore namespace).

## 10. `apps/task-worker`: worker state

- [ ] `Worker.hpp`: a `Worker` component (§4.2) — status (`Idle`/`Busy`)
      plus a remaining-ticks countdown, `Component`-concept-compatible
      (trivially copyable, standard layout — same constraint `Cell` in
      `apps/life` satisfies).
- [ ] App bootstrap seeds `N` worker entities into `World` (constructor
      parameter, mirroring `apps/life`'s `width`/`height`), each starting
      `Idle`.

## 11. `apps/task-worker`: tasks as jobs

- [ ] `TaskJob.hpp/.cpp`: implements `antwika::scheduler::IJob`; holds a
      task id, label, and duration-in-ticks by value; `execute(Tick)`
      claims the lowest-index currently-`Idle` worker (deterministic
      selection, not an unordered scan) and marks it `Busy` with its
      configured duration (§4.2). `TaskJob` itself carries no dependency
      state — dependencies are the `Scheduler`'s concern (item 4), not
      the job's.
- [ ] `TaskJobTest.cpp`: given a `World` with a known mix of idle/busy
      workers, `execute()` claims the correct (lowest-index idle) one
      and leaves the others untouched.

## 12. `apps/task-worker`: task submission

- [ ] `Events.hpp`: `task.submit` event name constant, mirroring
      `apps/life/include/antwika/life/Events.hpp`'s `life.toggle_cell`.
- [ ] `TaskSubmissionSink.hpp/.cpp`: `ITimedEventSink` implementation
      (mirrors `BoardSink`) — parses a `task.submit` payload (§4.3, §9's
      open question on exact encoding, including an optional
      `dependsOnId` field) into a `TaskJob` + `Priority` (+ dependency),
      constructs the job, and calls `Scheduler::schedule()`.
- [ ] `TaskSubmissionSink` keeps a lookup from the payload's own task id
      to the `JobId` `Scheduler::schedule()` actually returned for it
      (§4.3), so a later `task.submit`'s `dependsOnId` can be resolved
      to the right `JobId`; an unresolvable `dependsOnId` (referencing a
      task id never submitted) is a `TaskSubmissionSink`-level error,
      not a `Scheduler`-level one — decide and document the exact
      handling (e.g. an app-level error, or falling through to
      `Scheduler`'s `SchedulerError` if resolved to `kInvalidJobId`)
      when this is implemented.
- [ ] `TaskSubmissionSinkTest.cpp`: a known payload produces a `TaskJob`
      with the right id/priority/duration, scheduled at the right
      priority (mirrors `BoardSinkTest.cpp`); a payload with a
      `dependsOnId` resolves to the right `JobId` and is passed through
      to `Scheduler::schedule()`'s `dependsOn`.

## 13. `apps/task-worker`: systems and phases

- [ ] `WorkerCompletionSystem.hpp/.cpp` (`ISystem`): decrements every
      `Busy` worker's countdown; flips to `Idle` at `0` (§4.4 step 1).
- [ ] `TaskDispatchSystem.hpp/.cpp` (`ISystem`, app-layer per §3.8):
      counts idle workers via `world.view<Worker>()`, calls
      `scheduler.run(tick, idleCount)` (§4.4 step 2) — including any
      dependency-unblocked task that becomes ready within that same
      call, per §3.7's same-tick cascade.
- [ ] Bootstrap creates `"release"` phase (with
      `WorkerCompletionSystem`) before `"dispatch"` phase (with
      `TaskDispatchSystem`), in that order, mirroring `apps/life`'s
      `"life"`-before-`"observe"` phase ordering.
- [ ] Optional `"observe"` phase + a `PrintSystem`-equivalent reporting
      worker/task status per tick, following the `observers` parameter
      pattern from `antwika::life::bootstrap()` (§4.4).
- [ ] `WorkerCompletionSystemTest.cpp`: a worker with countdown `1`
      becomes `Idle` after one `update()`; a worker with countdown `2`
      is still `Busy` (countdown `1`) after one `update()`.
- [ ] `TaskDispatchSystemTest.cpp`: with a mock/fake `Scheduler`
      collaborator or a real `Scheduler` + `MockJob`s, assert `run()` is
      called with exactly the current idle-worker count as budget, no
      more, no less.

## 14. `apps/task-worker`: bootstrap and main

- [ ] `TaskWorker.hpp/.cpp`: `bootstrap()` function mirroring
      `antwika::life::bootstrap()`'s signature shape (§4.5) — clock,
      appender, formatter, log policy, event sink, replay source, total
      ticks, worker count, optional observers.
- [ ] `main.cpp`: `--record <path>` / `--replay <path>` CLI handling,
      mirroring `apps/life/src/main.cpp` exactly.
- [ ] `BootstrapTest.cpp`: end-to-end run against the §4.7 scenario
      (mixed priorities, multi-tick distribution, and a dependency edge
      that resolves both across a tick boundary and, if the scenario
      allows it, within a single tick), asserting final worker/task
      state.
- [ ] `ReplayIntegrationTest.cpp`: record then replay the §4.7 scenario,
      assert byte-identical recorded output both times (mirrors
      `apps/life/tests/ReplayIntegrationTest.cpp`).
- [ ] A hand-authored replay script (analogous to `apps/life`'s demo
      blinker replay) encoding the §4.7 scenario, checked in for the
      README's sample commands to reference.

## 15. `apps/task-worker` hygiene

- [ ] No line in `src/apps/task-worker/**/*.{hpp,cpp}` exceeds 80
      characters.
- [ ] Doxygen on every public class/method under
      `include/antwika/task-worker/` (directory name per §4.6; note the
      `#include <antwika/task-worker/...>` paths will contain a hyphen
      even though the namespace doesn't — confirm this compiles cleanly
      as an early scaffold step, since it's the one place directory
      hyphenation could bite, before building anything on top of it).
- [ ] `-Wall -Wextra -Wpedantic -Wsuggest-override -Werror` clean on GNU
      and LLVM; builds and runs on MinGW (`antwika_task_worker.exe` plus
      copied DLLs, matching `apps/game`/`apps/life`).
- [ ] `README.md`'s project-structure listing gains `task-worker/` under
      `apps/`, plus a short usage section (sample `--record`/`--replay`
      commands) parallel to the existing "Replays" section.

## 16. Requirements

- [ ] `REQUIREMENTS.md` gains Must-have lines: scheduler determinism
      (same run twice -> identical `run()` output, proven by test, not
      asserted by inspection — same phrasing style as the existing
      replay-determinism line), priority ordering (higher priority runs
      first, equal priority is FIFO), multi-tick distribution (`run()`'s
      `budget` parameter is the only mechanism, no job runs outside a
      `run()` call), and dependency chaining (a job with unmet
      dependencies is never dispatched; dependency cycles are
      unreachable through the public API, by construction, not by a
      runtime check).
- [ ] Consider a Should-have or Won't-have line documenting the
      no-anti-starvation limitation (§3.6) explicitly, so it reads as a
      decision in `REQUIREMENTS.md` too, not just in this plan.

## 17. Verify `.github/workflows/` updated (§7)

- [ ] `.github/workflows/build.yml`'s GNU/LLVM `expected=(...)` array
      (in the "Verify executables" step) includes
      `antwika_scheduler_tests`, `antwika_task_worker`, and
      `antwika_task_worker_tests`.
- [ ] The MinGW override `expected=(...)` in that same step includes
      `antwika_task_worker` alongside the existing `antwika_game`/
      `antwika_life`.
- [ ] The "Generate coverage report" step's `gcovr` invocation gains
      `--exclude '.*/apps/task-worker/src/main\.cpp'`, matching the
      existing `game`/`life` excludes.
- [ ] Re-read `ci.yml` and `release.yml` in full (not just skimmed, as
      this plan did) and confirm neither hardcodes a library/app name,
      path, or count that the new library/app would silently break or
      silently not be covered by; fix if either does.
- [ ] A full CI run (or a local dry run of the same steps `build.yml`
      performs — conan install, cmake configure/build, ctest, the
      verify-executables check) passes on a branch with the new
      library/app before this item is checked off.

## 18. Write the blog post

- [ ] Write a `blog/` post (next sequential number after the latest
      existing post — see `blog/`'s numbering convention) covering:
      what `antwika::scheduler` and `apps/task-worker` ended up being,
      the §3.8 ecs-dependency design question and why the
      `BoardSink`/`LifeSystem`-style app-layer adapter won, the §3.6/§3.7
      epoch-vs-same-tick-cascade rule and why dependency chaining needed
      it spelled out precisely, why dependency cycles need no detection
      code (§3.7's structural argument), and anything that changed from
      this plan once real code met it (name it explicitly, the way
      `blog/003-...md` calls out the entity-recycling subsystem that got
      cut mid-implementation and `blog/004-...md` calls out the
      `EngineLoop::onTick`-vs-events-vs-`ISystem` decision) — matching
      the precedent set by
      `blog/001-...md`/`blog/003-...md`/`blog/004-...md`.
- [ ] `README.md` links the new post from wherever it links the existing
      ones.
- [ ] Once the post is in and reviewed, delete `PLAN_SCHEDULER.md` and
      `PLAN_SCHEDULER_CHECKLIST.md` (this file) — matching the fate of
      the historical `docs/PLAN.md`/`docs/CHECKLIST.md`/`docs/NOTES.md`
      (commit `fda4164`), noted as a judgment call in `PLAN_SCHEDULER.md`
      §8 rather than a hard requirement of this checklist.
