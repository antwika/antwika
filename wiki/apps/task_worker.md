# apps/task_worker

`src/apps/task_worker/` — a worker pool pulling from the deterministic scheduler.

## What it demonstrates

[`ecs`](../libraries/ecs.md) and [`scheduler`](../libraries/scheduler.md) combined: a fixed pool of `Worker` entities pulling tasks off a priority-ordered, budget-bounded `Scheduler`, once per tick.
It has no window and no input, so it is the smallest complete illustration of the tick loop on its own.

## Running it

```sh
build/bin/antwika_task_worker/antwika_task_worker
build/bin/antwika_task_worker/antwika_task_worker --record demo.replay
build/bin/antwika_task_worker/antwika_task_worker --replay src/apps/task_worker/replays/demo.json
```

It prints worker status per tick and ends when the scripted or replayed input runs out.

## Libraries it composes

[`app`](../libraries/app.md), [`ecs`](../libraries/ecs.md), [`engine`](../libraries/engine.md), [`event`](../libraries/event.md), [`log`](../libraries/log.md), [`replay`](../libraries/replay.md), [`scheduler`](../libraries/scheduler.md), [`time`](../libraries/time.md).
No graphics and no input backend.

## How it is put together

- `task.submit` is the app's only event name; its payload is `id,priority,durationTicks,label[,dependsOnId]`.
- `TaskSubmissionSink` parses it and schedules a `TaskJob`, raising `TaskSubmissionError` on a malformed payload.
- `TaskRegistry` and `WorkerLookup` map between job ids and the entities running them.
- `TaskDispatchSystem` runs the scheduler once per tick, with that tick's idle-worker count as the budget.
- `WorkerCompletionSystem` retires a worker when its task's `durationTicks` have elapsed, and `StatusPrintSystem` prints the result.

## Non-obvious decisions

**The budget is the idle-worker count.**
Because `budget` is the scheduler's only throttle, passing the number of free workers means dispatch can never exceed the pool — the pool size is enforced by arithmetic rather than by a check that could disagree with reality.

**Dependency cycles are unreachable rather than detected.**
`dependsOn` can only name ids that already exist, and ids increase, so a cycle cannot be built through the public API.
There is no cycle check to write, to test or to pay for at runtime.

**Starvation is possible on purpose.**
A steady stream of higher-priority submissions can keep a low-priority task pending forever; unconditional priority respect is the requirement.

See [`blog/006-a-job-scheduler-and-a-worker-pool-that-cant-lie-to-itself.md`](../../blog/006-a-job-scheduler-and-a-worker-pool-that-cant-lie-to-itself.md).
