# apps/task_worker

`src/apps/task_worker/` — a worker pool pulling from the deterministic scheduler.

## What it demonstrates

[`ecs`](../libraries/ecs.md) and [`scheduler`](../libraries/scheduler.md) combined: a fixed pool of `Worker` entities pulling tasks off a deterministic, priority-ordered, budget-bounded `Scheduler`, once per tick.
Its window is a readout rather than a game, and the only thing a key ever reaches is the debug console: the smallest complete illustration of the tick loop, with the scheduler's promises drawn while they are being kept.

## Running it

```sh
build/bin/antwika_task_worker/antwika_task_worker
build/bin/antwika_task_worker/antwika_task_worker --record demo.replay
build/bin/antwika_task_worker/antwika_task_worker --replay src/apps/task_worker/replays/demo.jsonl
```

It draws the pool into a window, prints the same status per tick, and ends when the scripted or replayed input runs out -- or when the window is closed, which arrives as an `engine.stop` event through `simulation::WindowInputSource` and is therefore recorded and replayed like anything else.
The shipped `replays/demo.jsonl` runs eighteen ticks at 400 ms each, paced by a `simulation::TickPacer` so a queue can be watched draining rather than flashing past.
Under the default `null` backend the window draws nothing and the run is exactly the same run, printed.

## Libraries it composes

[`app`](../libraries/app.md), `antwika::console`, [`ecs`](../libraries/ecs.md), [`ecs_commons`](../libraries/ecs_commons.md), [`engine`](../libraries/engine.md), [`event`](../libraries/event.md), [`gfx`](../libraries/gfx.md), [`i18n`](../libraries/i18n.md), [`input`](../libraries/input.md), [`log`](../libraries/log.md), [`replay`](../libraries/replay.md), [`scheduler`](../libraries/scheduler.md), [`simulation`](../libraries/simulation.md), [`time`](../libraries/time.md), [`ui`](../libraries/ui.md).
A graphics backend and an input backend, the latter solely for the console: nothing else here reads a key or a pixel.

## How it is put together

- `task.submit` is the app's only event name; its payload is `id,priority,durationTicks,label[,dependsOnId]`.
- `TaskSubmissionSink` parses it and schedules a `TaskJob`, raising `TaskSubmissionError` on a malformed payload.
- `TaskRegistry` and `WorkerLookup` map between job ids and the entities running them.
- `TaskDispatchSystem` runs the scheduler once per tick, with that tick's idle-worker count as the budget, and records that budget in the registry.
- `WorkerCompletionSystem` retires a worker when its task's `durationTicks` have elapsed, and `StatusPrintSystem` prints the result.
- `snapshotOf()` takes a `PoolSnapshot` of the tick, `PoolScene` draws one, and `RenderSystem` is the "observe"-phase system joining the two to a window.

## What the window shows

Down the left, one card per worker: idle, or the task it holds with a bar of how far through it is and how many of its ticks are left.
Down the right, the pending queue in the order the scheduler will pull it, then the tasks that have finished.
Across the top, the tick, the budget that tick's dispatch was run with, and how many jobs it actually started.

## Non-obvious decisions

**The queue is the point of the picture, and its order is derived rather than kept.**
[`scheduler`](../libraries/scheduler.md) promises higher priority first and equal priority in submission order, and until there was a window that promise was only ever visible as an assertion in a test.
`snapshotOf()` rebuilds that order every tick from the registry alone -- highest priority first, ties left in the order they arrived, which is ascending `JobId` because a registry entry's index *is* its `JobId` less one.
Nothing is stored: a second list kept in step with the scheduler's own would be a copy that could drift, and the first thing it would misreport is exactly the thing being demonstrated.
A blocked task is drawn apart from the rest, named after what it waits for, because it is not a candidate for `run()` at all -- and `BootstrapTest.Bootstrap_DrawsTheQueueTheSchedulerWillActuallyPull` is what holds the drawn order against the run that produced it.

**The budget had to be observed, because nothing else keeps it.**
Every other number on screen is still in the registry when the tick is over; the budget is not, and it cannot be worked back out afterwards -- a tick with two idle workers and one ready job ends up looking exactly like a tick with one of each.
So `TaskDispatchSystem` tells the registry what it ran the scheduler with, at the point it already has the number, and the snapshot reads it back.
That is one observation added where a system already computes it, which is the alternative to a renderer working something out for itself.

**A task's duration is kept beside its countdown rather than derived from it.**
`TaskInfo::remainingTicks` is all that is left of the number a task was submitted with once it is Running, so "three of four ticks" cannot be recovered from it, and a bar drawn from the countdown alone would have no length to be a fraction of.

**The window is a projection and can be proved to be one.**
`PoolSnapshot` is a value: the scene is handed no `World`, no `TaskRegistry` and no `Scheduler`, so there is nothing it could write back to even by accident.
The bar is whole pixels of a track computed with integer arithmetic, so no floating-point value from the render side exists to leak into anything a replay reproduces, and two backends cannot round it apart.
`RenderSystem` lays out against `IWindow::configuredSize()` rather than the size the window reports; nothing here is hit-tested, so that costs nothing and keeps one rule across the applications instead of two.

**The console is the only interaction, deliberately.**
The run is a script, and the pool has nothing for a press to mean: the input backend and the `InputFold` exist solely so the debug console can be typed at, and where [`game`](game.md) wraps every input-reading sink in a `ConsoleGatedSink`, here there is nothing under the sheet for the console to take a press away from -- the comment at the sink list in `TaskWorker.cpp` says exactly that.
A run wired without a `consoleOverlay` registers neither the fold nor the console sink, so its event handling is byte for byte what it was before input existed.

**The printing stayed.**
`StatusPrintSystem` still runs every tick beside the renderer, rather than being dropped or made conditional on a headless backend, because the console output is what this application's tests read and what a `--replay` run is compared through.

**The budget is the idle-worker count.**
Because `budget` is the scheduler's only throttle, passing the number of free workers means dispatch can never exceed the pool — the pool size is enforced by arithmetic rather than by a check that could disagree with reality.

**Dependency cycles are unreachable rather than detected.**
`dependsOn` can only name ids that already exist, and ids increase, so a cycle cannot be built through the public API.
There is no cycle check to write, to test or to pay for at runtime.

**A worker's label is an [`ecs_commons`](../libraries/ecs_commons.md) `Name`, and this app is that library's first caller.**
`Worker::label` was a `std::array<char, 32>` with a truncating `makeWorkerLabel()` beside it, which is `Name` and `makeName()` written a second time -- same cap of 31 characters, same silent truncation, same reason for a fixed buffer rather than a `std::string` (an `ecs::Component` must stay trivially copyable and standard-layout).
The type is a drop-in and the call sites were not, which is the whole of what the migration had to be careful about: the old buffer always had a spare byte for a terminator, `Name`'s has none, so a label filling it exactly holds no NUL at all.
`StatusPrintSystem` therefore reads it with `view()` rather than streaming `label.text.data()`, which would have run off the end of the array, and `StatusPrintSystemTest.PrintsALabelThatExactlyFillsItsBuffer` is what pins that.
Nothing else about the app moved: the worker's own countdown was deliberately **not** an `ecs_commons` `Lifetime`, since that library's `LifetimeSystem` destroyed the entity at zero and a finished worker here goes `Idle` and waits for the next task.
That pair has since been pruned from `ecs_commons` -- this app declining to use it was part of the evidence that nothing did.

**Starvation is possible on purpose.**
A steady stream of higher-priority submissions can keep a low-priority task pending forever; unconditional priority respect is the requirement.

See [`blog/006-a-job-scheduler-and-a-worker-pool-that-cant-lie-to-itself.md`](../../blog/006-a-job-scheduler-and-a-worker-pool-that-cant-lie-to-itself.md).

## The debug console, and what a dump holds

**The grave key slides `antwika::console` over the pool, and `dump_state`/`load_state` snapshot and restore the whole run.**
The shell is the library's -- `ConsoleState`, `ConsoleScene`, `ConsolePicture`, `InputFold`, `ConsoleSink`, `SnapshotCommands` -- mounted exactly as [`game`](game.md) mounts it, with `FixedConsoleControls` because this app has no options screen to rebind from, and `RenderSystem` painting the picture last, over the pool.
`main()` stays branchless: `consoleLoadPermitted()` decides that a `--record` or `--replay` run answers `load_state` with a refusal line instead of a read.

**The full state is the Workers and the task bookkeeping; the scheduler's pending queue is rebuilt, never serialized.**
A `scheduler::Scheduler` job is a callable by design (see [`scheduler`](../libraries/scheduler.md)), so a dump cannot hold one: `TaskWorkerSnapshotStore` writes the worker components, the registry's tasks, the accepted submissions and the last dispatch, and a load re-`schedule`s a fresh `TaskJob` for every task the dump holds as Pending.
A task already Running keeps its worker's countdown -- the worker entry carries it, and the worker state is authoritative -- and a Completed task is not scheduled at all.

**A restore renumbers every JobId, and the invariant is stated where it is kept.**
The scheduler cannot be cleared and its ids only count up, so `JobQueue` swaps in a fresh one whose ids start at 1 again, dropping whatever the interrupted run still had pending.
The Pending tasks are re-scheduled in their original submission order, so the Nth Pending entry holds new JobId N -- and `TaskRegistry::restore()` records exactly that numbering, which is what keeps `markStarted()` resolving a renumbered id to the right task.
The submission sink's list is rebuilt with the same numbering, a no-longer-pending task marked `kInvalidJobId`, so a later `task.submit` depending on it schedules no edge: a dependency on a task that has already run is satisfied by definition.
An edge between two still-Pending tasks *is* re-expressed through the new ids, which is observable -- a loaded high-priority task that depends on a low-priority one still waits its turn.

**The dump file is the shared `console::SnapshotFormat` envelope under this app's own magic**, `antwika-task-worker-state-dump` at version 1, with the state validated by a local JSON schema and every persisted name -- `idle`/`busy`, `pending`/`running`/`completed` -- refused on the way in if this build does not know it.

## The config file

`config.json` beside the executable is read once at startup through [`antwika::config`](../libraries/config.md): `workerCount`, how many workers pull from the queue, and `tickIntervalMs`, how long a tick takes on the wall clock.
A missing file is the shipped application, and a broken one is refused at startup rather than repaired.
