# CHECKLIST: Job scheduler (`antwika::scheduler`) and `apps/task-worker`

Companion to `PLAN_SCHEDULER.md` — read that first, section numbers below
(e.g. "§3.5") refer to it. Step-by-step, in dependency order; later items
generally assume earlier ones are done.

## 0. `antwika::scheduler` scaffold

- [ ] Create `src/libs/scheduler/{CMakeLists.txt,include,src,tests}`.
- [ ] `include/antwika/scheduler/` directory created, empty.
- [ ] `add_library(antwika_scheduler ...)` + `antwika::scheduler` alias in
      `src/libs/scheduler/CMakeLists.txt`, matching the shape of
      `src/libs/ecs/CMakeLists.txt` (install rules, export set,
      `WINDOWS_EXPORT_ALL_SYMBOLS ON`).
- [ ] `target_link_libraries(antwika_scheduler PUBLIC antwika::time)` —
      just `time`, for `Tick` (§3.8 — no `log` dependency, unlike `ecs`).
- [ ] `add_subdirectory(scheduler)` added to `src/libs/CMakeLists.txt`.
- [ ] `find_package(GTest REQUIRED)` + `antwika_scheduler_tests`
      executable scaffolded in `src/libs/scheduler/tests/CMakeLists.txt`,
      registered with `gtest_discover_tests`.
- [ ] Project builds with zero `.cpp` files yet (empty lib compiles).

## 1. `JobId` and `Priority`

- [ ] `JobId.hpp`: `enum class JobId : std::uint64_t {}`,
      `kInvalidJobId{0}`; real jobs get `1` and up.
- [ ] `Priority.hpp`: scoped-enum-over-`std::uint8_t` per §3.1's default,
      with `kLowPriority`/`kNormalPriority`/`kHighPriority`/
      `kCriticalPriority` constants and a `rawValue()` helper — **or**
      the bare-alias alternative, per whichever way §9's open question
      is resolved; record the decision (a one-line comment pointing back
      at this checklist item is enough, no separate doc needed).
- [ ] Both types get a `rawValue()` free function mirroring
      `antwika::ecs::rawValue(Entity)`.

## 2. `IJob`

- [ ] `IJob.hpp`: `virtual ~IJob() = default;` +
      `virtual void execute(antwika::time::Tick tick) = 0;` — no other
      members (§3.2).

## 3. `Scheduler` core

- [ ] `Scheduler.hpp/.cpp`: `schedule(IJob &, Priority) -> JobId`,
      `run(Tick, std::size_t budget) -> std::vector<JobId>`,
      `pending() const -> std::size_t`, `empty() const -> bool`.
- [ ] `schedule()` stores a non-owning `IJob *`; `JobId`s are handed out
      by a monotonic counter starting at `1`, never reused.
- [ ] Internal ordering is a plain `std::vector` (sorted-on-insert or
      heap-maintained — §3.4/§9), comparator exactly:
      `priority desc, then id asc` (§3.5) — **no**
      `std::unordered_map`/`unordered_set` anywhere in the pending-job
      path.
- [ ] `run(tick, budget)` fixes its candidate set (the current top
      `budget` entries) before invoking any job, so a job scheduled by
      another job's `execute()` during this call is never included in
      it (§3.6) — implemented, not just documented.
- [ ] `run()` calls each selected job's `execute(tick)` in priority
      order, removes it from the queue, and returns the executed
      `JobId`s in that same order.
- [ ] `budget == 0` returns an empty vector and mutates nothing.
- [ ] `budget >= pending()` runs everything currently pending and
      returns a vector of that size — no error, no clamping surprise.

## 4. `Scheduler` unit tests (`SchedulerTest.cpp`)

- [ ] `schedule()` returns strictly increasing `JobId`s starting at `1`.
- [ ] `pending()`/`empty()` reflect the queue accurately through
      schedule/run cycles.
- [ ] Jobs scheduled in a deliberately "wrong" order (e.g. low priority
      first, high priority last) still `run()` highest-priority-first;
      equal-priority jobs run in the order they were `schedule()`d
      (§3.5) — assert the exact order, not just "high ran before low".
- [ ] `run()` never executes more than `budget` jobs in one call, even
      when more are pending.
- [ ] A job's `execute()` that calls `schedule()` on the same
      `Scheduler` does not get included in the `run()` call it was
      scheduled during; it's picked up by the following `run()` call
      (§3.6) — this is the trickiest behavior in the library, give it a
      dedicated test, not a side-assertion in another test.
- [ ] `budget == 0` test: no job runs, `pending()` unchanged, returned
      vector is empty.
- [ ] Starvation demo test (§3.6): schedule a `kLowPriority` job, then
      repeatedly schedule+run `kCriticalPriority` jobs with a budget
      that never lets the low-priority one through; assert it's still
      `pending()` at the end — proves the documented limitation is the
      actual behavior, not aspirational prose.
- [ ] `tests/mocks/include/antwika/scheduler/mocks/MockJob.hpp` (GMock
      `IJob` double) created and consumed by at least one `.cpp` test.

## 5. `SchedulerDeterminismTest.cpp`

- [ ] Fixed set of jobs (mixed priorities, known submission order),
      fixed sequence of `run(tick, budget)` calls across several ticks,
      executed twice from scratch (two fresh `Scheduler` instances, same
      inputs). Assert the two runs' full sequence of `run()` return
      values is identical — same shape as
      `src/libs/ecs/tests/EcsDeterminismTest.cpp` and
      `src/libs/replay/tests/ReplayDeterminismTest.cpp`.

## 6. `antwika::scheduler` hygiene

- [ ] No line in `src/libs/scheduler/**/*.{hpp,cpp}` exceeds 80
      characters (`scripts/check_line_length.py` covers `src/**`
      already).
- [ ] Doxygen `@brief`/`@param`/`@return` on every public class/method
      under `include/antwika/scheduler/`.
- [ ] `-Wall -Wextra -Wpedantic -Wsuggest-override -Werror` clean on GNU
      and LLVM toolchains.
- [ ] Coverage: GNU/LLVM builds pass with the new tests included; no new
      `GCOVR_EXCL_LINE` expected for this library (no fatal/terminating
      path like `EntityManager`'s) — if one turns out to be needed,
      follow `docs/confirming-unreachable-branches.md`'s procedure.
- [ ] `README.md`'s project-structure listing gains `scheduler/` under
      `libs/`.

## 7. `apps/task-worker` scaffold

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

## 8. `apps/task-worker`: worker state

- [ ] `Worker.hpp`: a `Worker` component (§4.2) — status (`Idle`/`Busy`)
      plus a remaining-ticks countdown, `Component`-concept-compatible
      (trivially copyable, standard layout — same constraint `Cell` in
      `apps/life` satisfies).
- [ ] App bootstrap seeds `N` worker entities into `World` (constructor
      parameter, mirroring `apps/life`'s `width`/`height`), each starting
      `Idle`.

## 9. `apps/task-worker`: tasks as jobs

- [ ] `TaskJob.hpp/.cpp`: implements `antwika::scheduler::IJob`; holds a
      task id, label, and duration-in-ticks by value; `execute(Tick)`
      claims the lowest-index currently-`Idle` worker (deterministic
      selection, not an unordered scan) and marks it `Busy` with its
      configured duration (§4.2).
- [ ] `TaskJobTest.cpp`: given a `World` with a known mix of idle/busy
      workers, `execute()` claims the correct (lowest-index idle) one
      and leaves the others untouched.

## 10. `apps/task-worker`: task submission

- [ ] `Events.hpp`: `task.submit` event name constant, mirroring
      `apps/life/include/antwika/life/Events.hpp`'s `life.toggle_cell`.
- [ ] `TaskSubmissionSink.hpp/.cpp`: `ITimedEventSink` implementation
      (mirrors `BoardSink`) — parses a `task.submit` payload (§4.3, §9's
      open question on exact encoding) into a `TaskJob` + `Priority`,
      constructs the job, and calls `Scheduler::schedule()`.
- [ ] `TaskSubmissionSinkTest.cpp`: a known payload produces a `TaskJob`
      with the right id/priority/duration, scheduled at the right
      priority (mirrors `BoardSinkTest.cpp`).

## 11. `apps/task-worker`: systems and phases

- [ ] `WorkerCompletionSystem.hpp/.cpp` (`ISystem`): decrements every
      `Busy` worker's countdown; flips to `Idle` at `0` (§4.4 step 1).
- [ ] `TaskDispatchSystem.hpp/.cpp` (`ISystem`, app-layer per §3.7):
      counts idle workers via `world.view<Worker>()`, calls
      `scheduler.run(tick, idleCount)` (§4.4 step 2).
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

## 12. `apps/task-worker`: bootstrap and main

- [ ] `TaskWorker.hpp/.cpp`: `bootstrap()` function mirroring
      `antwika::life::bootstrap()`'s signature shape (§4.5) — clock,
      appender, formatter, log policy, event sink, replay source, total
      ticks, worker count, optional observers.
- [ ] `main.cpp`: `--record <path>` / `--replay <path>` CLI handling,
      mirroring `apps/life/src/main.cpp` exactly.
- [ ] `BootstrapTest.cpp`: end-to-end run against the §4.7 scenario,
      asserting final worker/task state.
- [ ] `ReplayIntegrationTest.cpp`: record then replay the §4.7 scenario,
      assert byte-identical recorded output both times (mirrors
      `apps/life/tests/ReplayIntegrationTest.cpp`).
- [ ] A hand-authored replay script (analogous to `apps/life`'s demo
      blinker replay) encoding the §4.7 scenario, checked in for the
      README's sample commands to reference.

## 13. `apps/task-worker` hygiene

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

## 14. Requirements

- [ ] `REQUIREMENTS.md` gains Must-have lines: scheduler determinism
      (same run twice -> identical `run()` output, proven by test, not
      asserted by inspection — same phrasing style as the existing
      replay-determinism line), priority ordering (higher priority runs
      first, equal priority is FIFO), and multi-tick distribution
      (`run()`'s `budget` parameter is the only mechanism, no job runs
      outside a `run()` call).
- [ ] Consider a Should-have or Won't-have line documenting the
      no-anti-starvation limitation (§3.6) explicitly, so it reads as a
      decision in `REQUIREMENTS.md` too, not just in this plan.

## 15. Verify `.github/workflows/` updated (§7)

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

## 16. Write the blog post

- [ ] Write a `blog/` post (next sequential number after the latest
      existing post — see `blog/`'s numbering convention) covering:
      what `antwika::scheduler` and `apps/task-worker` ended up being,
      the §3.7 ecs-dependency design question and why the
      `BoardSink`/`LifeSystem`-style app-layer adapter won, the §3.6
      reentrant-scheduling-during-`run()` rule and why it's needed for
      determinism, and anything that changed from this plan once real
      code met it (name it explicitly, the way `blog/003-...md` calls
      out the entity-recycling subsystem that got cut mid-implementation
      and `blog/004-...md` calls out the `EngineLoop::onTick`-vs-events-
      vs-`ISystem` decision) — matching the precedent set by
      `blog/001-...md`/`blog/003-...md`/`blog/004-...md`.
- [ ] `README.md` links the new post from wherever it links the existing
      ones.
- [ ] Once the post is in and reviewed, delete `PLAN_SCHEDULER.md` and
      `PLAN_SCHEDULER_CHECKLIST.md` (this file) — matching the fate of
      the historical `docs/PLAN.md`/`docs/CHECKLIST.md`/`docs/NOTES.md`
      (commit `fda4164`), noted as a judgment call in `PLAN_SCHEDULER.md`
      §8 rather than a hard requirement of this checklist.
