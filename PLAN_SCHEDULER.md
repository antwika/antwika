# PLAN: Job scheduler (`antwika::scheduler`) and `apps/task-worker`

Planning doc for a new library at `src/libs/scheduler` (the `libs/scheduler`
referred to in the request — every existing library lives under
`src/libs/<name>`, so this follows that convention exactly) and a new demo
application at `src/apps/task-worker`.
Companion to `PLAN_SCHEDULER_CHECKLIST.md`.
Mirrors the shape of the historical `docs/PLAN.md` written for
`antwika::ecs` (see commit `caeaa23`): a library that owns a mechanism,
kept domain-agnostic, demonstrated end to end by an application.

No implementation has started yet — this is planning only.

## 1. Goal

Add a small, deterministic, single-threaded job scheduler library that:

- Runs a set of enqueued jobs in **priority order**, deterministically.
- **Distributes** a batch of enqueued jobs **across multiple ticks**,
  bounded by a per-call budget, rather than draining everything at once.
- Stays domain-agnostic (no dependency on `antwika::ecs`), the same way
  `antwika::ecs` stays domain-agnostic of `antwika::game`/`antwika::life`.

Then add `src/apps/task-worker`, a new demo application — a fixed pool of
workers pulling tasks off the scheduler, tasks submitted at various ticks
with mixed priorities — showing the library used for real, the same role
`apps/life` played for `antwika::ecs` (see `blog/003-...md`,
`blog/004-...md`).

## 2. Non-goals

Explicit scope cuts, each with a reason, matching this project's habit of
writing down *why* something is out rather than leaving it ambiguous:

- **No multithreading.** Every existing subsystem in this project
  (`SystemScheduler`, `EventDispatcher`, `EngineLoop`) is single-threaded
  and deterministic by construction; a job scheduler that ran jobs
  concurrently would reintroduce exactly the kind of nondeterminism the
  rest of the codebase goes out of its way to avoid. "Distribute across
  ticks" here means *time-sliced*, not *parallel*.
- **No job dependency graph / DAG chaining.** A job either is or isn't
  ready to run; there is no "job B waits on job A" concept in v1. This
  keeps `Scheduler` a priority queue with a budget, not a build-system
  scheduler. Can be revisited later as an additive feature if a real need
  shows up.
- **No cancellation.** Once scheduled, a job runs when its priority and
  the budget allow it to. Nothing in the request calls for cancelling a
  job mid-queue, and adding it now would be speculative.
- **No priority aging / anti-starvation.** A continuous stream of
  higher-priority jobs can, in principle, keep a low-priority job pending
  forever. That's a known, accepted, and *documented* limitation (see
  §3.6), not an oversight — anti-starvation would mean priority silently
  stops meaning what it says, which conflicts with "must respect job
  priorities." Deterministic starvation is still deterministic.
- **No built-in multi-tick job *duration*.** A job's `execute()` is one
  synchronous unit of work, called at most once, that runs to completion
  when picked — the same shape as `ISystem::update()`. A job that models
  work spanning several ticks (e.g. "this task keeps a worker busy for 3
  ticks") does so at the *application* level by driving its own state
  machine across ticks (see §4), not by the library re-invoking the job.
  This keeps the library's contract simple: "picked" and "done" are the
  same moment.
- **No dedicated `SchedulerError` type in v1.** Every existing
  project-specific error type (`EcsError`, `ReplayFormatError`) exists
  because a real, reachable misuse case needed one specific catchable
  type. Nothing in `Scheduler`'s v1 API (schedule/run/pending/empty) has
  an invalid-input case that isn't already a compile error (wrong types)
  or a no-op (e.g. `run()` with budget `0` does nothing, see §3.6) — so
  no error type is introduced speculatively. Revisit if a real case shows
  up (e.g. cancellation by `JobId` in a future version).
- **No dependency on `antwika::ecs`.** See §3.7 for the full reasoning —
  the ECS integration is demonstrated at the application layer instead,
  mirroring how `BoardSink`/`LifeSystem`/`PrintSystem` wire
  `antwika::ecs` into `antwika::event`/`antwika::replay` without either
  library depending on the other.
- **No RNG/PRNG**, per the project-wide "Won't have" in `REQUIREMENTS.md`.

## 3. `antwika::scheduler` core design

### 3.1 `JobId` and `Priority`

```cpp
enum class JobId : std::uint64_t {};
inline constexpr JobId kInvalidJobId{0};

enum class Priority : std::uint8_t {};
inline constexpr Priority kLowPriority{0};
inline constexpr Priority kNormalPriority{1};
inline constexpr Priority kHighPriority{2};
inline constexpr Priority kCriticalPriority{3};
```

Both follow `antwika::ecs::Entity`'s exact idiom: a scoped enum with no
enumerators over a primitive integer type — trivially copyable and
comparable, but not silently interchangeable with an unrelated integer —
plus free `constexpr` constants and a `rawValue()` helper, instead of a
conventional (and, in this codebase, unprecedented) `enum class Priority
{ Low, Normal, High, Critical }`. Higher raw value runs first. `Priority`
is not closed to just these four values — any `std::uint8_t` is valid,
the named constants are just the common cases, matching how
`antwika::time::Tick` is "just" a `std::uint64_t` with no enum wrapper at
all. (Whether `Priority` should be a bare alias like `Tick` instead of an
`Entity`-style scoped enum is called out as an open question for
implementation time — see §9.)

`JobId` is handed out by `Scheduler::schedule()` in strictly increasing
order (a monotonic counter, exactly like `EntityManager::create()`) and
doubles as the tie-break key for equal-priority jobs (§3.5) — no separate
sequence counter needed.

### 3.2 `IJob`

```cpp
class IJob
{
public:
    virtual ~IJob() = default;
    virtual void execute(antwika::time::Tick tick) = 0;
};
```

Deliberately the same shape as `antwika::ecs::ISystem`: one method, world
knowledge (or whatever a job needs) captured by the concrete
implementation at construction time, not passed in by the scheduler. The
scheduler does not know or care what a job does — same relationship
`SystemScheduler` has to `ISystem`.

### 3.3 `Scheduler`

```cpp
class Scheduler final
{
public:
    JobId schedule(IJob &job, Priority priority);

    std::vector<JobId> run(antwika::time::Tick tick, std::size_t budget);

    [[nodiscard]] std::size_t pending() const noexcept;
    [[nodiscard]] bool empty() const noexcept;

private:
    struct Entry
    {
        JobId id;
        Priority priority;
        IJob *job;
    };
    // Ordered by (priority desc, id asc) -- see 3.5.
    std::vector<Entry> queue;
    JobId nextId{1};
};
```

- `schedule()` stores a **non-owning** `IJob *` — same convention as
  `SystemScheduler::addSystem(PhaseId, ISystem &)`. Caller owns the job
  and must keep it alive until it's either run or the `Scheduler` is
  destroyed. No `std::function`/type erasure of ownership, no
  `unique_ptr` transfer — the job's lifetime is the caller's problem to
  manage, exactly as `ISystem &` is `SystemScheduler`'s.
- `run(tick, budget)` executes up to `budget` of the currently-pending,
  highest-priority-first jobs (calling each one's `execute(tick)`),
  removes them from the queue, and returns their `JobId`s in the order
  they ran. `tick` is forwarded to each job's `execute()` and otherwise
  unused by `Scheduler` itself — it isn't a step counter or a rate
  limiter, just context passed through, the same role it plays in
  `ISystem::update(World&, Tick)`.
- `budget` is a **parameter of `run()`**, not a constructor-time
  constant. A fixed per-tick capacity is one legitimate use (call `run`
  with the same budget every tick), but `apps/task-worker` needs a
  *varying* budget (the number of currently-idle workers, which changes
  tick to tick) — see §4. Keeping budget a call-time parameter supports
  both without forcing the varying case to route around a fixed
  constructor value.
- `budget == 0` is valid and a no-op: `run()` returns an empty vector,
  nothing executes, nothing is skipped or lost. This is how a tick with
  zero available capacity (e.g. zero idle workers) is expressed.

### 3.4 Storage and ordering — why this stays deterministic

No `std::unordered_map`/`unordered_set`, no pointer- or address-based
ordering, and no hashing anywhere in the pending-job path. `queue` is a
plain `std::vector<Entry>` kept in priority order; `run()` walks it front
to back. Concretely, either:

- keep it sorted on every `schedule()` via `std::stable_sort`/insertion
  in the right spot (queue sizes here are small — jobs pending at once,
  not accumulated forever), or
- use `std::make_heap`/`std::push_heap`/`std::pop_heap` with an explicit
  comparator over `(priority, id)`.

Both are fine determinism-wise, since ordering is a pure function of
`(priority, id)` — the actual implementation choice is a §9 detail, not
a design commitment this plan needs to lock down. What *is* locked down:
insertion order (`schedule()` call order, which is what `id` encodes) is
the only tie-break, so two runs that call `schedule()` in the same order
with the same priorities produce identical `run()` output, every time.

### 3.5 Priority ordering, precisely

Comparator: `a` sorts before `b` (i.e. `a` runs first) iff
`a.priority > b.priority`, or `a.priority == b.priority && a.id < b.id`.
In words: **higher priority first; equal priority is FIFO by submission
order.** This is the one sentence the whole "must respect job priorities"
requirement cashes out to, and it's exactly the kind of rule
`SchedulerTest` (§3.8/checklist) exists to pin down with an assertion,
not just a comment.

### 3.6 Distributing jobs across ticks

The mechanism is entirely `run()`'s `budget` parameter (§3.3): if more
jobs are pending than `budget` allows in one call, the excess simply
isn't touched — `queue` still holds them, in the same priority order,
ready for the next `run()` call. Calling `run()` once per tick with
`budget < pending()` is what spreads a burst of scheduled jobs across
however many ticks it takes to drain, highest priority first each time.

Two properties worth stating explicitly, because they're easy to get
wrong silently:

- **Reentrant scheduling during `run()` doesn't inflate that call's
  work.** If a job's `execute()` calls `Scheduler::schedule()` again
  (e.g. to enqueue a follow-up job), the newly scheduled job is **not**
  eligible for the `run()` call currently in progress — it becomes
  eligible starting with the *next* `run()` call. `run()` fixes its
  candidate set at the moment it decides what to execute (e.g. by
  capturing "top `budget` entries as of entry" before invoking any of
  them), not by re-checking the queue after each job runs. Without this
  rule, a job that reschedules itself could make one `run()` call do
  unbounded work, which would break both the "budget" contract and
  determinism (the amount of work done would depend on how deep a chain
  of self-rescheduling goes, in a single call).
- **No implicit starvation protection** (§2) — if jobs keep arriving at
  `kCriticalPriority` faster than budget drains them, a `kLowPriority`
  job can wait indefinitely. This is a direct, accepted consequence of
  "respect priority" being unconditional; `SchedulerTest` includes a
  case that demonstrates (not just states) this is the actual behavior,
  so it's a documented property, not a bug someone rediscovers later.

### 3.7 Relationship to `antwika::ecs` — how systems get scheduled

The request asks to "consider how systems (ECS) may be scheduled." Two
designs were weighed:

1. **`antwika::scheduler` depends on `antwika::ecs`**, e.g. a
   `SchedulerSystem : public antwika::ecs::ISystem` shipped *inside* the
   scheduler library, wrapping a `Scheduler` and draining it each tick
   from `update(World &, Tick)`.
2. **`antwika::scheduler` stays independent**; the ECS integration is a
   thin adapter written at the *application* layer, the same way
   `apps/life`'s `BoardSink`/`LifeSystem`/`PrintSystem` wire
   `antwika::ecs` into `antwika::event`/`antwika::replay` today without
   either of those libraries depending on `antwika::ecs`, or `antwika::
   ecs` depending on them.

**Chosen: (2).** Every existing library in this project depends only
"downward" toward small, genuinely shared mechanism (`ecs` → `time`,
`log`; `reducer` → `event`) and never toward another peer mechanism
library just to make one particular application's wiring shorter. Making
`scheduler` depend on `ecs` would be exactly that — a peer-to-peer
dependency motivated by one app's convenience — and it would make
`antwika::scheduler` unusable in a context with no `World` at all (e.g.
a plain script-like consumer), which contradicts §1's "domain-agnostic"
goal.

Concretely, `apps/task-worker` (§4) defines its own
`TaskDispatchSystem : public antwika::ecs::ISystem` (living under
`src/apps/task-worker`, not the library) whose `update(World &, Tick)`
computes that tick's budget from `World` state (idle worker count) and
calls `Scheduler::run(tick, budget)`. Registering `TaskDispatchSystem`
into a `SystemScheduler` phase is what makes "job dispatch" participate
in the same single-threaded, phase-ordered, deterministic tick as every
other system — the job scheduler becomes *a thing a system does*, not a
competing tick-driver bolted on beside `SystemScheduler`. This is the
concrete answer to "consider how systems may be scheduled": systems
schedule jobs; the job scheduler doesn't schedule systems.

### 3.8 Dependencies

`antwika::scheduler` depends on `antwika::time` only (for `Tick`) —
smaller than `antwika::ecs`'s dependency footprint (`time` + `log`),
since there's no fatal-exhaustion path here needing a logger (`JobId`'s
`std::uint64_t` range makes exhaustion practically unreachable, and
unlike `EntityManager` there's no per-instance ceiling to make it
reachable in a test either — this is a deliberate simplification, not an
oversight: entity exhaustion needed a *test-reachable* ceiling because
`World` is long-lived across a whole run, while a `Scheduler`'s `JobId`
space resets with the process and nothing in this project's scale gets
within a rounding error of 2⁶⁴ schedule() calls).

### 3.9 File layout

```
src/libs/scheduler/
├── CMakeLists.txt
├── include/antwika/scheduler/
│   ├── IJob.hpp
│   ├── JobId.hpp
│   ├── Priority.hpp
│   └── Scheduler.hpp
├── src/
│   └── Scheduler.cpp
└── tests/
    ├── CMakeLists.txt
    ├── SchedulerTest.cpp
    ├── SchedulerDeterminismTest.cpp
    └── mocks/
        ├── CMakeLists.txt
        └── include/antwika/scheduler/mocks/MockJob.hpp
```

Matches every other library's shape exactly (`CMakeLists.txt`,
`include/`, `src/`, `tests/`, `tests/mocks/include/...`), per
`REQUIREMENTS.md`'s "Each library and app must own its own
`CMakeLists.txt`, `include/`, `src/`, and `tests/` directory."

## 4. `apps/task-worker` design

### 4.1 Concept

A fixed pool of `N` workers and a stream of tasks, submitted over time at
mixed priorities, dispatched to idle workers by `antwika::scheduler`.
Same shape as `apps/life`: built on `antwika::ecs` (`World` +
`SystemScheduler` + phases), driven by the same `Engine` /
`EventDispatcher` / `TickedEventDispatcher` / `EngineLoop` /
`IReplaySource` machinery apps/game and apps/life already use, so
`--record`/`--replay` work identically (see `README.md`'s "Replays"
section) and the whole run is provably deterministic the same way
(`ReplayIntegrationTest`, `BootstrapTest` precedent from `apps/life`).

### 4.2 State: workers are entities, tasks are jobs

- **Workers are ECS state** (`World` entities with a `Worker` component:
  status `Idle`/`Busy` plus a remaining-ticks countdown when busy) —
  long-lived, simulated, exactly the kind of thing `World` exists for
  (compare `Cell` in `apps/life`).
- **Tasks are not entities.** A submitted task becomes a concrete
  `TaskJob : antwika::scheduler::IJob`, holding whatever it needs
  (task id, a human-readable label, a duration in ticks) by value, and
  is hedule`d into a `Scheduler`. Tasks are transient scheduling units,
  not long-lived simulated state, so representing them as plain `IJob`
  objects — not `World` entities/components — keeps "state
  representation is an application concern" pointed at the actual state
  (workers), rather than modeling something ECS doesn't need to model.

`TaskJob::execute(Tick)` is what actually claims a worker: given a
reference to the app's worker-lookup (see §4.4), it finds the
lowest-index idle worker (deterministic, not "first from an unordered
scan") and marks it `Busy` with its configured duration.

### 4.3 Submitting tasks: events, same pattern as `life.toggle_cell`

A `task.submit` event (payload e.g. `"id,priority,durationTicks,label"`,
mirroring the `"x,y"` payload shape of `life.toggle_cell`), tick-stamped
like every other event in this project. A `TaskSubmissionSink :
antwika::event::ITimedEventSink` (parallel to `BoardSink`) parses the
payload, constructs a `TaskJob`, and calls `Scheduler::schedule()` with
the parsed priority — all synchronously at dispatch time, before that
tick's `engine.tick` even fires, matching the "toggles genuinely
happened first" behavior documented in `blog/004-...md`. This is what
makes task submission itself replay-recordable: a hand-authored replay
script submits a mixed-priority burst of tasks (more than the worker
pool can run at once, deliberately, to exercise §3.6's multi-tick
distribution) across a few ticks.

### 4.4 Systems and phases

Two phases, created in this order (phase order is execution order, per
`SystemScheduler`'s documented contract):

1. **`"release"` phase** — `WorkerCompletionSystem` (an `ISystem`):
   decrements every `Busy` worker's remaining-ticks countdown; a worker
   reaching `0` flips back to `Idle`. Runs first so a worker that
   finishes *this* tick is immediately available to pick up new work
   *this same tick*, not one tick late.
2. **`"dispatch"` phase** — `TaskDispatchSystem` (an `ISystem`, defined
   in the app, not the library — see §3.7): counts currently-`Idle`
   workers via `world.view<Worker>()`, calls
   `scheduler.run(tick, idleCount)`. Each dispatched `TaskJob::execute()`
   claims one of those idle workers (§4.2), so the number of jobs
   `run()` executes never exceeds the number of workers actually free —
   the budget passed in *is* the invariant that keeps this correct.

An optional third **`"observe"` phase**, following `apps/life`'s
precedent exactly (`PrintSystem`-style), reporting worker status / task
completions per tick for demo visibility — independent of, and provably
non-interfering with, the other two phases, the same guarantee
`blog/004-...md`'s `CallCountingSystem` test demonstrates for `apps/life`.

### 4.5 Bootstrap shape

Mirrors `antwika::life::bootstrap()` (`src/apps/life/src/Life.cpp`)
closely: constructs `Logger`, `EventDispatcher`, `World`, seeds `N`
`Worker` entities, builds the `SystemScheduler` with its phases,
constructs `Scheduler`, wires `TaskSubmissionSink` into the
`TickedEventDispatcher` alongside the tick-driven `world.commit() +
scheduler.run(...)` step, then drives it all through the same `Engine` +
`EngineLoop` pair. `--record <path>` / `--replay <path>` supported
identically to `apps/game` and `apps/life`.

### 4.6 Naming

The request names the directory `apps/task-worker` (hyphenated) — kept
literally as `src/apps/task-worker`, even though every existing app
directory (`game`, `life`) is a single word. A hyphen isn't valid inside
a C++ namespace, so:

- Directory: `src/apps/task-worker` (as requested).
- CMake target / binary: `antwika_task_worker` (underscore, matching
  `antwika_game`/`antwika_life`'s `antwika_<name>` pattern).
- C++ namespace: `antwika::task_worker` (snake_case, the closest analog
  to `antwika::game`/`antwika::life` a hyphenated name allows).

### 4.7 Determinism & priority demo scenario (for the replay script and tests)

A small, fixed scenario, sized so it visibly exercises every requirement
at once: e.g. 2 workers, 6 tasks submitted across ticks 0 and 4 with
mixed priorities and durations, chosen so the worker pool can never run
more than 2 at a time (forcing multi-tick distribution, §3.6) and so at
least one later-submitted `kHighPriority` task provably jumps ahead of an
earlier-submitted `kLowPriority` one still pending (proving §3.5). Two
back-to-back runs from the same replay script must produce byte-identical
recorded output — the same style of proof
`SchedulerDeterminismTest`/`EcsDeterminismTest`/`ReplayDeterminismTest`
already use elsewhere in this codebase (run N ticks twice from scratch,
diff the observable result).

## 5. Testing strategy

- **`antwika::scheduler` unit tests**: `schedule()` returns strictly
  increasing `JobId`s; `pending()`/`empty()` track queue size correctly;
  `run()` respects the priority-then-FIFO order from §3.5 exactly (a
  test enqueuing jobs in a deliberately "wrong" order and asserting the
  execution order anyway); `run()` never executes more than `budget`;
  a job scheduled during another job's `execute()` is excluded from the
  in-progress `run()` call (§3.6) and picked up by the next one;
  `budget == 0` is a true no-op.
- **`SchedulerDeterminismTest`**: fixed set of jobs/priorities, run
  across N ticks with a fixed budget sequence, twice from scratch,
  assert identical `run()` return sequences both times — same shape as
  `EcsDeterminismTest`/`ReplayDeterminismTest`.
- **`tests/mocks/include/antwika/scheduler/mocks/MockJob.hpp`**: a
  GMock-based `IJob` double, consumed by at least one `.cpp` test (per
  `scripts/check_unused_test_doubles.py`).
- **`apps/task-worker` unit tests**: `WorkerCompletionSystem` frees a
  worker exactly when its countdown reaches zero, not before/after;
  `TaskDispatchSystem` never dispatches more tasks than there are idle
  workers; `TaskSubmissionSink` parses a `task.submit` payload into the
  right `TaskJob`/priority (mirrors `BoardSinkTest`).
  `BootstrapTest`/`ReplayIntegrationTest` mirroring `apps/life`'s,
  proving `--record` then `--replay` reproduce identical output for the
  scenario in §4.7.
- No new interface needs a mock purely for testability beyond `IJob` —
  `Scheduler` itself is a concrete `final` class used directly in tests,
  the same way `SystemScheduler`/`World` are today (cheap to construct,
  deterministic, no I/O — nothing to gain from mocking it, unlike
  `IEventDispatcher`/`IEngine`, which exist as interfaces because real
  implementations have side effects worth substituting away in a test).

## 6. Cross-cutting / hygiene (must all hold before this is "done")

- No line in `src/libs/scheduler/**/*.{hpp,cpp}` or
  `src/apps/task-worker/**/*.{hpp,cpp}` exceeds 80 characters
  (`scripts/check_line_length.py`).
- Doxygen `@brief`/`@param`/`@return` on every public class/method under
  `include/antwika/scheduler/` (and public app-layer classes, matching
  existing app conventions).
- Comments/markdown prose: one sentence per line
  (`scripts/check_one_sentence_per_line.py` — covers `src/**/*.{hpp,cpp}`
  already; not currently wired to root-level planning docs like this
  one, but this document follows the convention anyway).
- No `std::unordered_map`/`unordered_set` (or anything else whose
  iteration order isn't a documented, stable invariant) anywhere
  iteration order could leak into scheduling behavior.
- No RNG/PRNG anywhere in `antwika::scheduler` or `apps/task-worker`.
- `-Wall -Wextra -Wpedantic -Wsuggest-override -Werror` clean on GNU and
  LLVM toolchains; `apps/task-worker` builds and runs (with the DLL-copy
  step) on MinGW too, matching `apps/game`/`apps/life`.
- `README.md`'s project-structure listing gains `scheduler/` under
  `libs/` and `task-worker/` under `apps/`, plus a short usage blurb
  parallel to the existing "Replays" section if `apps/task-worker`
  supports `--record`/`--replay` (it does, see §4.5).
- `REQUIREMENTS.md` gains Must-have lines for: scheduler determinism,
  priority ordering, multi-tick budget distribution — mirroring how the
  ECS work updated `REQUIREMENTS.md` (checklist item, see
  `PLAN_SCHEDULER_CHECKLIST.md`).
- Coverage: GNU/LLVM builds pass with new tests included;
  `apps/task-worker/src/main.cpp` gets the same gcovr `--exclude` as
  `apps/game/src/main.cpp`/`apps/life/src/main.cpp` (see §7).

## 7. CI / `.github/workflows/` impact

`.github/workflows/build.yml`'s `apps` job hardcodes an `expected=(...)`
array of binaries that must exist after the build (see the "Verify
executables" step) — this **must** be updated, or CI will not catch a
missing `antwika_scheduler_tests`/`antwika_task_worker`/
`antwika_task_worker_tests` binary (silently passing) rather than failing
loudly:

- Add `antwika_scheduler_tests`, `antwika_task_worker_tests` to the
  GNU/LLVM `expected` array.
- Add `antwika_task_worker` to *both* the GNU/LLVM array and the MinGW
  override array (MinGW only expects the two app binaries today:
  `antwika_game`, `antwika_life`).
- Add `--exclude '.*/apps/task-worker/src/main\.cpp'` next to the
  existing `game`/`life` excludes in the "Generate coverage report" step.

`ci.yml` and `release.yml` were not read line-by-line yet in this
planning pass but are expected to need no changes (they call `build.yml`
generically and don't name individual binaries or paths) — confirming
that, or fixing it if wrong, is checklist item's job, not this plan's.

## 8. Documentation impact

- `README.md`: project-structure tree, and a short "Task worker" section
  parallel to "Replays" if there's something worth showing at the
  command line (e.g. sample output of a run).
- `REQUIREMENTS.md`: new Must-have lines (§6).
- A `blog/` post, written **after** the implementation lands, following
  the exact precedent set by `blog/003-...md`/`blog/004-...md`: what was
  built, what design questions came up along the way (the §3.7
  ecs-dependency question is a natural one to write up, since it mirrors
  the `BoardSink`/`LifeSystem` precedent so directly), and what, if
  anything, changed from this plan once real code met it. This is the
  checklist's actual final item.
- Once implemented and the blog post lands, this file and
  `PLAN_SCHEDULER_CHECKLIST.md` should be deleted, matching the fate of
  the historical `docs/PLAN.md`/`docs/CHECKLIST.md`/`docs/NOTES.md` (see
  commit `fda4164`) — noted here, left as a judgment call for whoever
  does the implementation, since these two files live at the repo root
  rather than in `docs/`, per this task's explicit instruction, and
  deleting them isn't itself a requirement of this task.

## 9. Open questions for implementation time

Flagged rather than silently decided, so implementation can resolve them
with real code in hand instead of this plan guessing:

- Whether `Priority` should be an `Entity`-style scoped enum (§3.1, this
  plan's default) or a bare `using Priority = std::uint8_t;` like `Tick`
  — both are precedented in this codebase for different reasons
  (`Entity` needs to *not* mix with other integers; `Tick` doesn't
  bother). Priority arguably leans `Entity`-like (a `Priority` and a
  `Tick` should never be interchangeable), but it's a judgment call.
- Whether `Scheduler`'s internal ordering is a kept-sorted `vector` or a
  binary heap (§3.4) — purely an implementation detail with no
  observable behavior difference; pick whichever reads simpler once the
  tests in §5 exist to hold it to its contract.
- Exact `task.submit` payload encoding (§4.3) — comma-separated like
  `life.toggle_cell`, or something more structured — is a detail for
  whoever writes `TaskSubmissionSink`/its test first.
