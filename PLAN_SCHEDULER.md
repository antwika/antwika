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

**Revision note:** this plan originally scoped job dependency chaining
out as a non-goal (v1) and left `Priority`'s exact representation as an
open question. Both were revisited after this plan's first pass: job
dependency/DAG chaining is now in scope (§3.7), and `Priority` is
decided as an `Entity`-style scoped enum (§3.1, no longer open).

## 1. Goal

Add a small, deterministic, single-threaded job scheduler library that:

- Runs a set of enqueued jobs in **priority order**, deterministically.
- **Distributes** a batch of enqueued jobs **across multiple ticks**,
  bounded by a per-call budget, rather than draining everything at once.
- Lets jobs form a **dependency graph** ("chain" one job after another),
  so a job only becomes runnable once every job it depends on has run.
- Stays domain-agnostic (no dependency on `antwika::ecs`), the same way
  `antwika::ecs` stays domain-agnostic of `antwika::game`/`antwika::life`.

Then add `src/apps/task-worker`, a new demo application — a fixed pool of
workers pulling tasks off the scheduler, tasks submitted at various ticks
with mixed priorities and, in at least one case, a dependency on an
earlier task — showing the library used for real, the same role
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
- **No cancellation.** Once scheduled, a job runs when its priority,
  dependencies, and the budget allow it to. Nothing in the request calls
  for cancelling a job mid-queue, and adding it now would be speculative.
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
  same moment. (Dependency chaining, §3.7, is a different thing from
  this — it sequences *distinct* jobs, it doesn't make one job's
  `execute()` span ticks.)
- **No dependency-cycle detection algorithm.** Not because cycles are
  allowed — they're structurally impossible, by construction, so there's
  nothing for an algorithm to detect. See §3.7.
- **No dependency on `antwika::ecs`.** See §3.8 for the full reasoning —
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
the named constants are just the common cases.

**Decided:** `Priority` is `Entity`-style, not a bare alias like `Tick`.
The reasoning that settles it: `Tick` is deliberately allowed to mix with
"just a `std::uint64_t`" because nothing about a tick value needs
protecting from accidental misuse as a different kind of integer, whereas
`Priority` and `JobId` are both small integers that a caller could very
easily transpose by accident at a call site (`schedule(job, someId)`
instead of `schedule(job, somePriority)`) if either were bare aliases —
exactly the kind of mix-up `Entity`'s scoped-enum wrapper exists to catch
at compile time. Both `JobId` and `Priority` get the wrapper; `Tick`
stays as it is elsewhere in the codebase, unaffected by this decision.

`JobId` is handed out by `Scheduler::schedule()` in strictly increasing
order (a monotonic counter, exactly like `EntityManager::create()`) and
doubles as the tie-break key for equal-priority jobs (§3.5) — no separate
sequence counter needed. It also doubles as the mechanism that makes
dependency cycles structurally impossible (§3.7).

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
    JobId schedule(
        IJob &job,
        Priority priority,
        std::vector<JobId> dependsOn = {});

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
    // Ready to run, ordered by (priority desc, id asc) -- see §3.5.
    std::vector<Entry> ready;
    // Waiting on unmet dependencies -- see §3.7.
    std::vector<Entry> blocked;
    std::vector<std::size_t> unmetCount;   // indexed by id - 1
    std::vector<std::vector<JobId>> dependents; // indexed by id - 1
    std::vector<bool> completed;           // indexed by id - 1
    JobId nextId{1};
};
```

(The private layout above is illustrative of what `run()`/`schedule()`
need to stay deterministic and `O(dependents)` per completion, not a
literal implementation mandate — see §3.4 and §3.7 for what's actually
locked down versus left to implementation.)

- `schedule()` stores a **non-owning** `IJob *` — same convention as
  `SystemScheduler::addSystem(PhaseId, ISystem &)`. Caller owns the job
  and must keep it alive until it's either run or the `Scheduler` is
  destroyed. No `std::function`/type erasure of ownership, no
  `unique_ptr` transfer — the job's lifetime is the caller's problem to
  manage, exactly as `ISystem &` is `SystemScheduler`'s.
- `dependsOn` defaults to empty (no dependencies — every existing
  §3.3-shaped call site with two arguments keeps compiling unchanged).
  Each entry must be a `JobId` this same `Scheduler` has already handed
  out (i.e. returned from an earlier `schedule()` call) — see §3.7 for
  what happens otherwise.
- `run(tick, budget)` executes up to `budget` of the currently-*ready*
  (all dependencies met), highest-priority-first jobs (calling each
  one's `execute(tick)`), and returns their `JobId`s in the order they
  ran. Blocked jobs are never candidates, regardless of priority — see
  §3.7. `tick` is forwarded to each job's `execute()` and otherwise
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
- `pending()` counts every job that has been `schedule()`d but hasn't
  `run()` yet, **ready or blocked** — a job with unmet dependencies still
  counts as pending, it just isn't a candidate for `run()` yet.
  `empty() == (pending() == 0)`.
- **`SchedulerError`** (mirrors `EcsError`/`ReplayFormatError`; see
  `EcsError.hpp` for the exact one-type, `std::runtime_error`-derived
  shape to copy): thrown by `schedule()` if `dependsOn` contains a
  `JobId` this `Scheduler` never issued (`kInvalidJobId`, an id from a
  *different* `Scheduler` instance, or a value nothing constructed via a
  real `schedule()` call). This reverses this plan's original v1
  decision to skip a dedicated error type — dependency chaining
  introduces the first real invalid-input case the API has, so it earns
  the same treatment every other such case in this codebase gets: one
  specific, catchable type, not a vague exception.
- Listing the same `JobId` twice in `dependsOn` is harmless, not a
  double-wait: `schedule()` de-duplicates `dependsOn` before computing
  how many dependencies are unmet.

### 3.4 Storage and ordering — why this stays deterministic

No `std::unordered_map`/`unordered_set`, no pointer- or address-based
ordering, and no hashing anywhere in the pending-job path — for either
the ready queue or the dependency-tracking structures introduced by
§3.7. `ready` is a plain `std::vector<Entry>` kept in priority order;
`run()` walks it in that order. Concretely, either:

- keep it sorted on every insertion via `std::stable_sort`/insertion in
  the right spot (queue sizes here are small — jobs pending at once, not
  accumulated forever), or
- use `std::make_heap`/`std::push_heap`/`std::pop_heap` with an explicit
  comparator over `(priority, id)`.

Both are fine determinism-wise, since ordering is a pure function of
`(priority, id)` — the actual implementation choice is a §9 detail, not
a design commitment this plan needs to lock down. What *is* locked down:
insertion order (`schedule()` call order, which is what `id` encodes) is
the only tie-break, so two runs that call `schedule()` in the same order
with the same priorities and dependencies produce identical `run()`
output, every time.

The dependency bookkeeping (§3.7) is index-by-`JobId` (a plain, growable
`std::vector`, since `JobId`s are dense and start at `1` — no gaps, no
reuse, so "index by id" never wastes more than one unused slot at index
`0`), not hash-map-by-`JobId` — same determinism argument, same
mechanism `World` already applies to its `Entity`-keyed pools being
type-indexed-but-never-order-sensitive.

### 3.5 Priority ordering, precisely

Comparator: `a` sorts before `b` (i.e. `a` runs first) iff
`a.priority > b.priority`, or `a.priority == b.priority && a.id < b.id`.
In words: **higher priority first; equal priority is FIFO by submission
order.** This applies only among jobs that are actually *ready* (§3.7) —
a blocked job, however high its priority, is simply not part of this
ordering until it becomes ready. This is the one sentence the whole
"must respect job priorities" requirement cashes out to, and it's
exactly the kind of rule `SchedulerTest` (§5/checklist) exists to pin
down with an assertion, not just a comment.

### 3.6 Distributing jobs across ticks

The mechanism is entirely `run()`'s `budget` parameter (§3.3): if more
jobs are ready than `budget` allows in one call, the excess simply isn't
touched — they stay ready, in the same priority order, candidates for
the next `run()` call. Calling `run()` once per tick with
`budget < pending()` is what spreads a burst of scheduled jobs across
however many ticks it takes to drain, highest priority first each time.

One property worth stating precisely, because it interacts directly with
§3.7's "same-tick cascading" behavior and is easy to get subtly wrong:

**Only jobs known to the `Scheduler` before a given `run()` call began
are ever eligible to execute *during* that call — no matter what
triggers their readiness.** Concretely: `run()` captures
`const auto epoch = nextId;` at entry; only entries with `id < epoch` are
ever picked, this call or any future one won't re-consider a lower id
twice. A job whose `execute()` calls `schedule()` to enqueue a brand-new
follow-up job gets an id `>= epoch` for that new job — structurally
excluded from the *current* `run()` call, picked up starting with the
*next* one. This is what keeps a self-rescheduling (or mutually
rescheduling) job from being a hidden way to make one `run()` call do
open-ended work: the set of ids it could possibly touch is fixed the
moment the call starts, full stop, regardless of how many jobs schedule
follow-ups along the way.

This rule is *not* the same as forcing every dependency chain to take
one tick per hop — see §3.7 for why a chain of *pre-existing* jobs (all
with `id < epoch` already, just waiting on each other) can legitimately
drain across several hops within a single `run()` call, budget
permitting. The distinction is "was this job's existence already known
when `run()` started," not "was this job already ready when `run()`
started."

Separately: **no implicit starvation protection** (§2) — if jobs keep
arriving at `kCriticalPriority` faster than budget drains them, a
`kLowPriority` job can wait indefinitely. This is a direct, accepted
consequence of "respect priority" being unconditional; `SchedulerTest`
includes a case that demonstrates (not just states) this is the actual
behavior, so it's a documented property, not a bug someone rediscovers
later.

### 3.7 Job dependencies and chaining (DAG)

A job can declare it depends on one or more already-scheduled jobs via
`schedule()`'s `dependsOn` parameter (§3.3); it only becomes *ready* —
and therefore only becomes a candidate for `run()`'s priority ordering
(§3.5) — once every one of those has run.

**Why cycles are structurally impossible, with no detection algorithm
needed.** A caller can only put a `JobId` into `dependsOn` if they
already hold one — and the only way to hold one is as the return value
of an earlier, already-completed `schedule()` call. Since `JobId`s are
handed out by a strictly increasing counter (§3.1), any dependency a job
`B` declares necessarily has a smaller id than `B`'s own id (which `B`
doesn't even know until its `schedule()` call returns). An edge can
therefore only ever point from a higher id to a lower one — a cycle
would require the reverse, which is unreachable through the public API.
This is the same style of guarantee `EntityManager`'s "no recycling"
rule gives for entity handles (see `docs/PLAN.md`'s §3.1, historical):
removing the *possibility* of the bad case, instead of adding a runtime
check for it (compare: `EntityManager` doesn't check for ABA reuse bugs,
because indices are never reused, so there's nothing to check for).

**Mechanics.** `schedule(job, priority, dependsOn)`:

1. Assigns the new job's `id` (as always).
2. De-duplicates `dependsOn`, then rejects (throws `SchedulerError`, see
   §3.3) any entry that isn't a `JobId` this `Scheduler` actually issued.
3. For each remaining dependency: if it's already `completed`, it
   doesn't count toward the new job's unmet total. Otherwise, the new
   job's unmet-dependency count increments by one, and the new job's id
   is recorded as a dependent of that dependency.
4. If the unmet count ends at `0` (no dependencies, or all already
   satisfied), the job goes straight into `ready`, ordered as usual
   (§3.5). Otherwise it goes into `blocked`, invisible to `run()` until
   unblocked.

When `run()` executes a job and it completes, the `Scheduler` marks it
`completed` and looks up its recorded dependents: each one's unmet count
decrements by one, and any that reach `0` move from `blocked` to `ready`
— in priority order, same as any other insertion into `ready` (§3.4).

**Same-tick cascading, and why it's safe.** A job moved from `blocked`
to `ready` *during* a `run()` call this way **is** eligible for that same
call's remaining budget — this is what makes chaining actually useful: a
ready dependency chain (`A` → `B` → `C`) can drain in one `run()` call if
the budget allows, instead of being forced to trickle one hop per tick
regardless of capacity. This doesn't conflict with §3.6's epoch rule,
because it's bounded by a different, already-established limit: every
job a completion could possibly unblock was *already known* to the
`Scheduler` (already has `id < epoch`, per §3.6) — its dependency edges
were fixed back when it was first `schedule()`d, before this `run()`
call ever started. Cascading through a pre-existing chain can therefore
never do more than `budget` executions total (the loop is bounded by a
simple counter, not by how deep the chain runs), and can never touch a
job whose existence wasn't already known at call-start — the exact same
guarantee §3.6 makes for non-dependency jobs, just exercised via
unblocking instead of via fresh scheduling.

**Diamonds, not just chains.** Nothing above is linear-chain-specific: a
job `D` that depends on both `B` and `C` (which both depend on `A`) is
handled by the same unmet-count mechanism with no special case — `D`'s
unmet count starts at `2`, decrements independently as `B` and `C` each
complete (in whatever order the scheduler picks them, per priority), and
`D` becomes ready only once both have. This is a real DAG, not just a
list of chains, matching the request's own wording ("job dependency
graph / DAG chaining").

**A permanently-blocked dependency blocks its dependents forever, and
that's fine.** If a job's dependency is itself starved (§3.6/§2 — never
wins enough budget against higher-priority competition) or was itself
scheduled with an unmet dependency that never resolves, anything
depending on it stays blocked indefinitely too. No special-cased timeout
or detection is added for this — it's the direct, expected consequence
of "ready" meaning what it says, the same way unconditional priority
respect already accepts indefinite low-priority starvation as a known
property, not a bug.

### 3.8 Relationship to `antwika::ecs` — how systems get scheduled

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

### 3.9 Dependencies of the library itself

`antwika::scheduler` depends on `antwika::time` only (for `Tick`) —
smaller than `antwika::ecs`'s dependency footprint (`time` + `log`),
since there's no fatal-exhaustion path here needing a logger (`JobId`'s
`std::uint64_t` range makes exhaustion practically unreachable, and
unlike `EntityManager` there's no per-instance ceiling to make it
reachable in a test either — this is a deliberate simplification, not an
oversight: entity exhaustion needed a *test-reachable* ceiling because
`World` is long-lived across a whole run, while a `Scheduler`'s `JobId`
space resets with the process and nothing in this project's scale gets
within a rounding error of 2⁶⁴ schedule() calls). `SchedulerError`
(§3.3) needs only `<stdexcept>`, exactly like `EcsError`, so it doesn't
add a dependency either.

### 3.10 File layout

```
src/libs/scheduler/
├── CMakeLists.txt
├── include/antwika/scheduler/
│   ├── IJob.hpp
│   ├── JobId.hpp
│   ├── Priority.hpp
│   ├── SchedulerError.hpp
│   └── Scheduler.hpp
├── src/
│   └── Scheduler.cpp
└── tests/
    ├── CMakeLists.txt
    ├── SchedulerTest.cpp
    ├── SchedulerDependencyTest.cpp
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
mixed priorities (and, in at least one case, with a dependency on an
earlier task), dispatched to idle workers by `antwika::scheduler`. Same
shape as `apps/life`: built on `antwika::ecs` (`World` + `SystemScheduler`
+ phases), driven by the same `Engine` / `EventDispatcher` /
`TickedEventDispatcher` / `EngineLoop` / `IReplaySource` machinery
apps/game and apps/life already use, so `--record`/`--replay` work
identically (see `README.md`'s "Replays" section) and the whole run is
provably deterministic the same way (`ReplayIntegrationTest`,
`BootstrapTest` precedent from `apps/life`).

### 4.2 State: workers are entities, tasks are jobs

- **Workers are ECS state** (`World` entities with a `Worker` component:
  status `Idle`/`Busy` plus a remaining-ticks countdown when busy) —
  long-lived, simulated, exactly the kind of thing `World` exists for
  (compare `Cell` in `apps/life`).
- **Tasks are not entities.** A submitted task becomes a concrete
  `TaskJob : antwika::scheduler::IJob`, holding whatever it needs
  (task id, a human-readable label, a duration in ticks) by value, and
  is `schedule`d into a `Scheduler`, optionally with `dependsOn` set to
  an earlier task's `JobId` (§3.7). Tasks are transient scheduling
  units, not long-lived simulated state, so representing them as plain
  `IJob` objects — not `World` entities/components — keeps "state
  representation is an application concern" pointed at the actual state
  (workers), rather than modeling something ECS doesn't need to model.

`TaskJob::execute(Tick)` is what actually claims a worker: given a
reference to the app's worker-lookup (see §4.4), it finds the
lowest-index idle worker (deterministic, not "first from an unordered
scan") and marks it `Busy` with its configured duration.

### 4.3 Submitting tasks: events, same pattern as `life.toggle_cell`

A `task.submit` event (payload e.g.
`"id,priority,durationTicks,label[,dependsOnId]"`, mirroring the `"x,y"`
payload shape of `life.toggle_cell`, with the dependency field optional
so most submissions stay as simple as `life.toggle_cell`'s), tick-stamped
like every other event in this project. A `TaskSubmissionSink :
antwika::event::ITimedEventSink` (parallel to `BoardSink`) parses the
payload, constructs a `TaskJob`, and calls `Scheduler::schedule()` with
the parsed priority (and, if present, the referenced task's `JobId` as
its sole dependency) — all synchronously at dispatch time, before that
tick's `engine.tick` even fires, matching the "toggles genuinely
happened first" behavior documented in `blog/004-...md`. This is what
makes task submission itself replay-recordable: a hand-authored replay
script submits a mixed-priority burst of tasks (more than the worker
pool can run at once, deliberately, to exercise §3.6's multi-tick
distribution, and including a dependency edge to exercise §3.7) across a
few ticks.

`TaskSubmissionSink` needs to translate a submitted task's own numbering
(whatever id scheme the replay script/payload uses) to the `JobId`
`Scheduler::schedule()` actually returned for it, so a later
`dependsOnId` field can be resolved to the right `JobId` — a small
lookup the sink owns, not something `Scheduler` needs to know about.

### 4.4 Systems and phases

Two phases, created in this order (phase order is execution order, per
`SystemScheduler`'s documented contract):

1. **`"release"` phase** — `WorkerCompletionSystem` (an `ISystem`):
   decrements every `Busy` worker's remaining-ticks countdown; a worker
   reaching `0` flips back to `Idle`. Runs first so a worker that
   finishes *this* tick is immediately available to pick up new work
   *this same tick*, not one tick late.
2. **`"dispatch"` phase** — `TaskDispatchSystem` (an `ISystem`, defined
   in the app, not the library — see §3.8): counts currently-`Idle`
   workers via `world.view<Worker>()`, calls
   `scheduler.run(tick, idleCount)`. Each dispatched `TaskJob::execute()`
   claims one of those idle workers (§4.2), so the number of jobs
   `run()` executes never exceeds the number of workers actually free —
   the budget passed in *is* the invariant that keeps this correct. A
   dependent task that becomes ready mid-`run()` (§3.7, because the task
   it depended on happened to be dispatched earlier in the very same
   `run()` call) can claim a worker in that same tick too, as long as
   idle workers (i.e. remaining budget) are still available.

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

See `ISSUES.md` for the naming-consistency question this raises,
flagged for a maintainer decision independent of this plan.

### 4.7 Determinism, priority, and dependency demo scenario

A small, fixed scenario, sized so it visibly exercises every requirement
at once: e.g. 2 workers, 6 tasks submitted across ticks 0 and 4 with
mixed priorities and durations, chosen so the worker pool can never run
more than 2 at a time (forcing multi-tick distribution, §3.6); at least
one later-submitted `kHighPriority` task provably jumping ahead of an
earlier-submitted `kLowPriority` one still pending (proving §3.5); and at
least one task submitted with a `dependsOn` on an earlier task, chosen so
the scenario proves *both* halves of §3.7 — one case where the
dependency resolves in a *later* tick (the dependent stays blocked across
a tick boundary), and, if it fits naturally, one case where it resolves
within the *same* `run()` call (same-tick cascade). Two back-to-back runs
from the same replay script must produce byte-identical recorded output
— the same style of proof
`SchedulerDeterminismTest`/`EcsDeterminismTest`/`ReplayDeterminismTest`
already use elsewhere in this codebase (run N ticks twice from scratch,
diff the observable result).

## 5. Testing strategy

- **`antwika::scheduler` unit tests (`SchedulerTest.cpp`)**: `schedule()`
  returns strictly increasing `JobId`s; `pending()`/`empty()` track queue
  size correctly (including blocked jobs, §3.3); `run()` respects the
  priority-then-FIFO order from §3.5 exactly (a test enqueuing jobs in a
  deliberately "wrong" order and asserting the execution order anyway);
  `run()` never executes more than `budget`; a job scheduled during
  another job's `execute()` is excluded from the in-progress `run()`
  call (§3.6) and picked up by the next one; `budget == 0` is a true
  no-op; a starvation case (§3.6) demonstrating a low-priority job can
  wait indefinitely against continuous higher-priority arrivals.
- **`SchedulerDependencyTest.cpp`** (§3.7): a job with one unmet
  dependency doesn't run until that dependency has; a diamond (`D`
  depends on `B` and `C`, both depending on `A`) resolves correctly
  regardless of `B`/`C`'s relative priority; a chain that's fully
  pre-existing and ready-except-for-ordering drains across several hops
  within one `run()` call when budget allows (the same-tick cascade,
  proven by assertion, not just described); a job newly `schedule()`d
  *during* another job's `execute()` with a dependency on a job that
  completes in that same call is still excluded from that call (the
  epoch rule from §3.6 applies regardless of dependency involvement); an
  unknown/foreign `JobId` in `dependsOn` throws `SchedulerError`; the
  same `JobId` listed twice in `dependsOn` behaves identically to once.
- **`SchedulerDeterminismTest.cpp`**: fixed set of jobs/priorities/
  dependencies, run across N ticks with a fixed budget sequence, twice
  from scratch, assert identical `run()` return sequences both times —
  same shape as `EcsDeterminismTest`/`ReplayDeterminismTest`.
- **`tests/mocks/include/antwika/scheduler/mocks/MockJob.hpp`**: a
  GMock-based `IJob` double, consumed by at least one `.cpp` test (per
  `scripts/check_unused_test_doubles.py`).
- **`apps/task-worker` unit tests**: `WorkerCompletionSystem` frees a
  worker exactly when its countdown reaches zero, not before/after;
  `TaskDispatchSystem` never dispatches more tasks than there are idle
  workers; `TaskSubmissionSink` parses a `task.submit` payload into the
  right `TaskJob`/priority/dependency (mirrors `BoardSinkTest`), including
  translating a payload's `dependsOnId` to the right `JobId`.
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
  `include/antwika/scheduler/` (including `SchedulerError`) and public
  app-layer classes, matching existing app conventions.
- Comments/markdown prose: one sentence per line
  (`scripts/check_one_sentence_per_line.py` — covers `src/**/*.{hpp,cpp}`
  already; not currently wired to root-level planning docs like this
  one, but this document follows the convention anyway).
- No `std::unordered_map`/`unordered_set` (or anything else whose
  iteration order isn't a documented, stable invariant) anywhere
  iteration order could leak into scheduling or dependency-resolution
  behavior.
- No RNG/PRNG anywhere in `antwika::scheduler` or `apps/task-worker`.
- `-Wall -Wextra -Wpedantic -Wsuggest-override -Werror` clean on GNU and
  LLVM toolchains; `apps/task-worker` builds and runs (with the DLL-copy
  step) on MinGW too, matching `apps/game`/`apps/life`.
- `README.md`'s project-structure listing gains `scheduler/` under
  `libs/` and `task-worker/` under `apps/`, plus a short usage blurb
  parallel to the existing "Replays" section if `apps/task-worker`
  supports `--record`/`--replay` (it does, see §4.5).
- `REQUIREMENTS.md` gains Must-have lines for: scheduler determinism,
  priority ordering, multi-tick budget distribution, and dependency
  chaining (jobs with unmet dependencies aren't dispatched; cycles are
  unreachable through the public API) — mirroring how the ECS work
  updated `REQUIREMENTS.md` (checklist item, see
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
`ISSUES.md` also flags that `.vscode/tasks.json` carries a duplicate of
the gcovr exclude list, outside `.github/workflows/`'s scope — worth
fixing alongside this item even though it isn't the checklist's literal
scope.

## 8. Documentation impact

- `README.md`: project-structure tree, and a short "Task worker" section
  parallel to "Replays" if there's something worth showing at the
  command line (e.g. sample output of a run).
- `REQUIREMENTS.md`: new Must-have lines (§6).
- A `blog/` post, written **after** the implementation lands, following
  the exact precedent set by `blog/003-...md`/`blog/004-...md`: what was
  built, what design questions came up along the way (the §3.8
  ecs-dependency question and the §3.7 same-tick-cascade-vs-epoch-rule
  design are both natural things to write up, since the second one only
  became necessary once dependency chaining was added mid-plan — a real
  example of the plan changing shape once a requirement was refined, not
  just once code met it), and what, if anything, changed from this plan
  once real code met it. This is the checklist's actual final item.
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

- Whether `Scheduler`'s internal ready-queue ordering is a kept-sorted
  `vector` or a binary heap (§3.4) — purely an implementation detail with
  no observable behavior difference; pick whichever reads simpler once
  the tests in §5 exist to hold it to its contract.
- Exact `task.submit` payload encoding (§4.3) — comma-separated like
  `life.toggle_cell`, or something more structured — is a detail for
  whoever writes `TaskSubmissionSink`/its test first.
- Whether the dependency bookkeeping (`unmetCount`/`dependents`, §3.3)
  is best modeled as parallel `id`-indexed vectors (as sketched) or a
  single `std::vector<Entry>` where `Entry` itself carries this state —
  an implementation detail with no effect on the determinism or
  cycle-impossibility guarantees, which hold regardless of the concrete
  layout chosen.
