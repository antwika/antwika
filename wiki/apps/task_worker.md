# apps/task_worker

`src/apps/task_worker/` — a worker pool pulling from the deterministic scheduler.

## What it demonstrates

[`ecs`](../libraries/ecs.md) and [`scheduler`](../libraries/scheduler.md) combined: a fixed pool of `Worker` entities pulling tasks off a deterministic, priority-ordered, budget-bounded `Scheduler`, once per tick.
It has no window and no input, so it is the smallest complete illustration of the tick loop on its own.

## Running it

```sh
build/bin/antwika_task_worker/antwika_task_worker
build/bin/antwika_task_worker/antwika_task_worker --record demo.replay
build/bin/antwika_task_worker/antwika_task_worker --replay src/apps/task_worker/replays/demo.json
```

It prints worker status per tick and ends when the scripted or replayed input runs out.

## Libraries it composes

[`app`](../libraries/app.md), [`ecs`](../libraries/ecs.md), [`ecs_commons`](../libraries/ecs_commons.md), [`engine`](../libraries/engine.md), [`event`](../libraries/event.md), [`log`](../libraries/log.md), [`replay`](../libraries/replay.md), [`scheduler`](../libraries/scheduler.md), [`time`](../libraries/time.md).
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

**A worker's label is an [`ecs_commons`](../libraries/ecs_commons.md) `Name`, and this app is that library's first caller.**
`Worker::label` was a `std::array<char, 32>` with a truncating `makeWorkerLabel()` beside it, which is `Name` and `makeName()` written a second time -- same cap of 31 characters, same silent truncation, same reason for a fixed buffer rather than a `std::string` (an `ecs::Component` must stay trivially copyable and standard-layout).
The type is a drop-in and the call sites were not, which is the whole of what the migration had to be careful about: the old buffer always had a spare byte for a terminator, `Name`'s has none, so a label filling it exactly holds no NUL at all.
`StatusPrintSystem` therefore reads it with `view()` rather than streaming `label.text.data()`, which would have run off the end of the array, and `StatusPrintSystemTest.PrintsALabelThatExactlyFillsItsBuffer` is what pins that.
Nothing else about the app moved: the worker's own countdown is deliberately **not** a `Lifetime`, since `LifetimeSystem` destroys the entity at zero and a finished worker here goes `Idle` and waits for the next task.

**Starvation is possible on purpose.**
A steady stream of higher-priority submissions can keep a low-priority task pending forever; unconditional priority respect is the requirement.

See [`blog/006-a-job-scheduler-and-a-worker-pool-that-cant-lie-to-itself.md`](../../blog/006-a-job-scheduler-and-a-worker-pool-that-cant-lie-to-itself.md).
