# Running until stop, and the greeting that was never really input

*Post 7*

The [previous post](006-a-job-scheduler-and-a-worker-pool-that-cant-lie-to-itself.md) closed out `apps/task_worker` and a scheduler that can't lie to itself about what it's already done.
This post isn't a new library or a new app — it's a change to the engine's own loop, from "run a known number of ticks" to "run until something says stop," and a second self-generated event that turned out to have been hiding in plain sight since before this repository had a name for the problem.

## The ask, and the loop it replaced

`EngineLoop::run(Tick totalTicks)` had one job: count from 0 to `totalTicks`, dispatching that tick's events and stepping the engine each time.
Every demo script, every test, every `bootstrap()` call had to know its own length in advance — a number decided by the caller, not a property of anything that happened during the run.
The ask was to make the run's length a consequence of what actually happens instead: keep going until something dispatches a stop, the same way a real interactive process would.

## `engine.stop`, kept symmetric with `engine.tick` — but living somewhere different

The obvious shape was a second built-in event, `engine.stop`, watched by a small sink the same way `GameStateReducer` watches `game.score_increment`.
`StopSignal` is exactly that: an `ITimedEventSink` with one `bool`, registered into `TickedEventDispatcher`'s sink list next to whatever the application already has there, no special-casing anywhere in the pipeline for it being "the" stop event instead of just another one.
It couldn't live in `antwika::event` next to `ITimedEventSink`, though, the way its symmetry with `kTick` might suggest — `kTick` and `kStop` are both `antwika::engine` concepts, and `antwika::engine` depends on `antwika::event`, not the other way around.
`StopSignal` lives in `antwika::engine` instead, which `antwika::replay` already depended on for `IEngine`, so `EngineLoop` taking one as a parameter added no new dependency at all.

## Checking after the tick, not before

`EngineLoop::run()` now takes `(const StopSignal &stop, std::optional<Tick> maxTicks = std::nullopt)` and loops until `stop.stopped()` — checked *after* `engine.step(tick)` runs, not before and not in between dispatching that tick's events.
That ordering is the whole reason a replayed run reaches the same terminal tick a live one did: the tick carrying the stop event still runs to completion, in full, before the loop exits, so nothing about that tick differs between the two runs.
`maxTicks` is a safety valve, not a domain concept — omit it and a run goes until something stops it; pass one and `EngineLoopError` fires the moment it's exceeded without a stop, loud instead of a hung test.
Production call sites (`main.cpp`, all three apps) pass nothing; every test that doesn't itself dispatch a stop passes a generous cap, specifically so a forgotten `engine.stop` fails a build instead of hanging one.

## The bug that was already there

Recording a `--record` file used to be trivial, in the way that only works when it's cheating: the whole input script was known before the run even started, so `main.cpp` just serialized *that* — never mind what had actually been dispatched.
An unbounded run has no such script to fall back on, so recording had to become real: register a `ReplayRecorder` as another timed sink, capture what's actually dispatched, write that out once the run stops.
The first version of that did exactly what [post 1](001-building-a-deterministic-replay-system.md) already warned against, word for word — it recorded `engine.tick`, which `Engine::step()` regenerates fresh every run, live or replayed, and is therefore never really input no matter how faithfully it gets observed.
Feeding that recording straight back in doubled `ticksProcessed` on replay: once from the engine regenerating it, once from the replay source dispatching the recorded copy.
The fix was the same shape as post 1's: filter `engine.tick` out before writing, so what gets persisted is the *input* a replay needs, not the *history* a full observer happens to see.

That fix surfaced a second instance of the exact same bug, and it had been sitting there since long before this feature existed.
`Game::run()` (and `Life::run()`, and `TaskWorker::run()`) dispatches a startup announcement — `Event{.name = "Running Antwika Game"}` — unconditionally, on every `bootstrap()` call, live or replayed, for no reason other than demonstrating the event pipeline works.
Nothing has ever reacted to that event's name, so nobody had reason to notice it's self-generated in exactly the way `engine.tick` is: recording it and feeding it back would double-dispatch it on replay too, silently, with zero observable effect only because no sink currently cares what it says.
`stripSelfGeneratedEvents()` in each `main.cpp` filters both now, with a comment explaining why, so the next event anyone adds to that unconditional startup call doesn't quietly reopen the same gap a third time.

## Three coverage requests, three real gaps

Getting to 100% line coverage on `apps/game/src/Game.cpp` and `apps/life/src/Life.cpp` meant testing the one branch nothing had exercised yet: `bootstrap()` called with a non-null `replayRecorder`.
Writing that test is what surfaced the startup-greeting bug above — the first version of the test's expected event list was wrong, and chasing *why* it was wrong is what found it, not a separate investigation.
`apps/task_worker/src/TaskWorker.cpp` had the identical untested branch and got the identical test, on the theory that a gap worth closing in two files is worth closing in the third one quietly sharing its shape, even though only two were asked for.
A fourth followed the same pattern one layer down: `EngineLoop.cpp`'s `maxTicks.has_value() && tick >= *maxTicks` had never been evaluated with `maxTicks` actually omitted — the exact "production callers run uncapped" case the whole feature exists to support was, until then, only ever exercised by the real binaries, never by a test gcov could see.

## Where it ended up

- `antwika::engine::events::kStop` and `StopSignal`, living next to `kTick` for the same reason: both are built-in, both flow through the ordinary `ITimedEventSink` pipeline, neither is special-cased anywhere.
- `EngineLoop::run(stop, maxTicks = nullopt)` replaces the fixed-tick loop; `EngineLoopError` is the loud failure when a test's safety cap is hit without a stop.
- `bootstrap()` across `apps/game`, `apps/life`, and `apps/task_worker` gained an optional `replayRecorder` parameter, since a run's actual length — and therefore its recorded input — is no longer knowable in advance.
- `stripSelfGeneratedEvents()`, one per app, filtering `engine.tick` and that app's startup announcement out of what gets written to a `--record` file — the same principle as post 1, applied a second time to an event nobody had thought of as "input" before.
- 100% line and branch coverage (gcovr's own CI flags) across every file this change touched, reached in four passes, each one closing a gap the previous pass's fix had made visible.
