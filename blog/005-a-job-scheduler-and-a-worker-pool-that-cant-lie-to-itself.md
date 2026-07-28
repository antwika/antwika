# A job scheduler, and a worker pool that can't lie to itself

*Post 5*

The [previous post](004-a-game-of-life-demo-and-a-queue-nobody-was-reading.md) closed out `apps/life` and a queue that turned out to have no reader.
This post adds a new mechanism library, `antwika::scheduler`, and a new demo application, `apps/task-worker`, that puts it to work — the same "library owns a mechanism, an app demonstrates it end to end" shape `antwika::ecs`/`apps/life` established, applied to a different problem: running a batch of prioritized jobs across ticks instead of simulating a board.

## What `antwika::scheduler` had to guarantee

The request was for a deterministic job scheduler that respects priority, spreads work across ticks instead of draining everything at once, and lets jobs depend on each other.
`JobId` and `Priority` both ended up as `Entity`-style scoped enums over a plain integer — not because a bare `std::uint64_t`/`std::uint8_t` wouldn't work, but because `schedule(job, someId)` and `schedule(job, somePriority)` are exactly the kind of call-site mix-up that idiom exists to catch at compile time.
`IJob` is deliberately one method, the same shape as `antwika::ecs::ISystem`: the scheduler doesn't know or care what a job does, only when it's allowed to run.

`Scheduler::run(tick, budget)` is where all three requirements meet.
Priority ordering is a single comparator — higher priority first, equal priority FIFO by `JobId`, which doubles as submission order since ids are handed out by a strictly increasing counter.
Multi-tick distribution needs no separate mechanism at all: if more jobs are ready than `budget` allows, the rest simply aren't touched this call, staying ready for the next one.

## The epoch rule, and why dependencies made it non-negotiable

The interesting design work was making sure a job's `execute()` can't turn one `run()` call into unbounded work.
`run()` captures `epoch = nextId` the moment it starts; only jobs that already existed before that instant are ever candidates, no matter what happens during the call:

```cpp
const auto it = std::find_if(
    ready.begin(),
    ready.end(),
    [epoch](const Entry &entry)
    { return rawValue(entry.id) < epoch; });
```

A job that schedules a follow-up job during its own `execute()` gets that follow-up excluded from the current call, full stop — even if the follow-up's dependencies are already satisfied and it lands in the ready queue with capacity to spare.

Job dependencies (`schedule()`'s `dependsOn`) initially looked like they might need a separate rule, since a dependent job *becoming* ready mid-`run()` — via a same-tick cascade through a pre-existing chain — is exactly the kind of "work appearing during the call" the epoch rule exists to bound.
It doesn't need a separate rule, because the epoch check is already phrased in terms of the job's *id*, not its *ready state*: a chain `A → B → C`, all three `schedule()`d before `run()` ever starts, can drain in one call because every id in it is `< epoch`, regardless of how many hops it takes to get there.
`SchedulerDependencyTest.cpp`'s `EpochRuleHoldsForNewlyScheduledDependentJobs` test pins down the case that would have been easy to get backwards: a job scheduled *during* another job's `execute()`, with a `dependsOn` that resolves within that same call, is still excluded — the epoch rule and the cascade rule compose correctly instead of one silently overriding the other.

## No cycle-detection code, and that's not an oversight

`dependsOn` can only ever contain a `JobId` the caller already holds, and the only way to hold one is as the return value of an earlier `schedule()` call.
Since ids increase strictly, every dependency edge a job declares necessarily points to a smaller id than its own — a cycle would require an edge pointing the other way, which the public API has no way to construct.
There's no detection algorithm in `Scheduler` because there's nothing to detect: the acyclicity is structural, the same style of guarantee `EntityManager`'s "no recycling" rule gives for entity handles, removing the bad case instead of checking for it at runtime.

## Where `antwika::ecs` and `antwika::scheduler` meet — and don't

`antwika::scheduler` depends on `antwika::time` only, not on `antwika::ecs`.
The alternative — a `SchedulerSystem : ISystem` shipped inside the library, wrapping a `Scheduler` and draining it every tick — was considered and rejected for the same reason `antwika::ecs` doesn't depend on `antwika::reducer` or vice versa: every library in this project depends only "downward" toward genuinely shared mechanism, never sideways toward a peer library just to shorten one application's wiring.
Making `scheduler` depend on `ecs` would also make it unusable in a context with no `World` at all, which contradicts the whole point of keeping it domain-agnostic.

`apps/task-worker` is where the two actually meet, at the application layer, mirroring `apps/life`'s `BoardSink`/`LifeSystem` split exactly: `TaskDispatchSystem : ISystem` lives in the app, not the library, and its `update(World&, Tick)` computes that tick's idle-worker count and calls `Scheduler::run(tick, idleCount)`.
Registering it into a `"dispatch"` phase (after a `"release"` phase that frees finished workers) is the concrete answer to "how do systems get scheduled": systems schedule jobs, the job scheduler doesn't schedule systems.

## The one thing that changed once code met the plan: `WorkerLookup`

The plan described `TaskJob::execute()` as claiming "the lowest-index currently-idle worker" by reading `World` directly.
That doesn't quite work as written, and the reason is `World`'s own double-buffering discipline: `World::set()` stages into the back buffer, invisible via `get()`/`view()` until the owning phase commits.
Within a single `TaskDispatchSystem::update()` call, `Scheduler::run()` can dispatch several `TaskJob`s back to back, each one calling `execute()` before that phase ever commits — so a second `TaskJob` reading `World` mid-dispatch would see the exact same "idle" snapshot the first one did, and claim the same worker.

`WorkerLookup` closes that gap: it mirrors `World`'s committed `Worker` state in a plain cache, refreshed once per tick before dispatch begins, and mutated immediately as each `TaskJob` claims a worker — while still calling `world.set()` so the state `WorkerCompletionSystem` sees after commit stays correct.
It's a small addition, not a different design — the plan's own wording, "a reference to the app's worker-lookup," already anticipated needing *something* in this spot; `WorkerLookup` is what that something turned out to be once the double-buffering interaction was worked through with real code.

## The demo scenario

`apps/task-worker`'s replay script (`main.cpp`'s `demoScript()`, also what `BootstrapTest`/`ReplayIntegrationTest` run against) uses 2 workers and 5 tasks, deliberately sized to hit every requirement:

```
tick 0: Alpha (Normal, 4 ticks), Beta (Normal, 5 ticks), Gamma (Low, 1 tick)
tick 4: Delta (Critical, 1 tick), Epsilon (Normal, 1 tick, depends on Delta)
```

Both workers are claimed by Alpha and Beta at tick 0, so Gamma — lower priority, and there from the start — sits pending for four ticks: multi-tick distribution, not asserted, actually observed.
When Alpha's worker frees at tick 4, Delta (submitted that same tick, higher priority) wins the slot over Gamma, which has been waiting since tick 0 — the priority-jump case.
Epsilon depends on Delta and becomes ready the moment Delta completes, but the budget for that `run()` call is already spent, so Epsilon has to wait for tick 5 — a dependency resolving across a tick boundary.
The same-tick cascade case (a fully pre-existing chain draining in one `run()` call) is proven at the library level instead, in `SchedulerDependencyTest.cpp`'s `PreExistingChainCascadesInOneRunCall` — it didn't fit naturally into a 2-worker demo scenario without either forcing an artificial third worker or making the script harder to read for no added proof value the library test doesn't already carry.

## Where it ended up

- `antwika::scheduler`: `JobId`, `Priority`, `IJob`, `SchedulerError`, and `Scheduler` — priority-ordered, budget-bounded, dependency-aware, depending only on `antwika::time`. 16 tests across `SchedulerTest.cpp`, `SchedulerDependencyTest.cpp`, and `SchedulerDeterminismTest.cpp`.
- `apps/task-worker`: a 2-phase (`"release"`/`"dispatch"`, plus an optional `"observe"`) `antwika::ecs` application wiring `antwika::scheduler` in through a `TaskDispatchSystem`, with `--record`/`--replay` identical to `apps/game`/`apps/life`.
- `WorkerLookup`: the one piece of real design that only became necessary once the plan's "app's worker-lookup" phrase met `World`'s actual double-buffering semantics.
- 181 tests passing, 100% line coverage on every new file, no `std::unordered_map`/`unordered_set` anywhere in the pending-job or dependency-tracking path, no cycle-detection code because there's nothing to detect.
