# antwika::scheduler

`src/libs/scheduler/` — deterministic job dispatch.

## What it is for

Holding jobs until they are ready, then running them in a defined order, a bounded number at a time.
Running the same sequence of `schedule()` and `run()` calls twice from scratch produces identical output both times, which is a tested requirement rather than a hope.

## Key types

| Header | Type | Role |
| --- | --- | --- |
| `Scheduler.hpp` | `Scheduler` | `schedule(...) -> JobId`, `run(time::Tick, budget) -> std::vector<JobId>`, `pending()`, `empty()`. |
| `IJob.hpp` | `IJob` | What a scheduled unit of work implements. |
| `JobId.hpp` | `JobId` | An `enum class : std::uint64_t` handle, also the dependency key. |
| `Priority.hpp` | `Priority` | An `enum class : std::uint8_t`; higher runs first. |
| `SchedulerError.hpp` | `SchedulerError` | Scheduling misuse, such as naming a dependency that does not exist. |

`MockJob` lives under `tests/mocks/`.

## Depends on

[`time`](time.md) only.

## Non-obvious decisions

**`budget` is the only throttle.**
No job runs outside a `run()` call, and `run()` never exceeds the budget it was given.
`apps/task_worker` passes the count of idle workers as the budget, so dispatch can never outrun the pool.

**Dependency cycles are unreachable by construction, not caught at runtime.**
`schedule()` can only depend on ids that already exist, and ids increase, so a cycle cannot be expressed through the public API.
There is therefore no cycle detector to test, and no runtime check to pay for.

**No priority aging and no anti-starvation.**
A continuous stream of higher-priority jobs can keep a lower-priority one pending indefinitely.
That is the specified behaviour — unconditional respect for priority — rather than a bug left unfixed.
Equal priorities run FIFO by submission order, which is what makes repeated runs identical.

See [`blog/006-a-job-scheduler-and-a-worker-pool-that-cant-lie-to-itself.md`](../../blog/006-a-job-scheduler-and-a-worker-pool-that-cant-lie-to-itself.md).
