# A Game of Life demo, and a queue nobody was reading

*Post 4*

The [previous post](003-an-entity-component-system-with-nowhere-to-hide-a-mutation.md) shipped `antwika::ecs` as a library with zero applications actually using it end to end.
This post closes that gap with `apps/life`, a Conway's Game of Life demo, and then wanders into a side quest that started as "remove one log line" and ended with an entire interface deleted from the codebase.
Both parts are true to how the work actually happened — the second one exists because a direct question stopped a half-fix before it shipped.

## A second app, built entirely on `antwika::ecs`

`apps/game` only ever exercised the reducer/replay path: a hand-rolled `GameStateReducer` folding events into a plain `GameState` struct.
`apps/life` is the first application wired to `World`, `SystemScheduler`, and phases from end to end.
A `Cell` component holds one bool; a `Grid` maps `(x, y)` coordinates to entities, created once in row-major order.
`LifeSystem` applies Conway's rule every tick, and it does this without any manual snapshotting of its own:

```cpp
void LifeSystem::update(World &world, antwika::time::Tick)
{
    for (std::uint32_t y = 0; y < grid.height(); ++y)
    {
        for (std::uint32_t x = 0; x < grid.width(); ++x)
        {
            const auto entity = grid.entityAt(x, y);
            const auto neighbors = countAliveNeighbors(world, x, y);
            const auto wasAlive = world.get<Cell>(entity).alive;
            const auto nowAlive = wasAlive
                                       ? (neighbors == 2 || neighbors == 3)
                                       : (neighbors == 3);
            world.set<Cell>(entity, Cell{.alive = nowAlive});
        }
    }
}
```

Every `world.get<Cell>` call here reads the same front-buffer snapshot for the whole pass, and every `world.set<Cell>` call writes into the back buffer, so no cell's already-computed next generation can leak into another cell's neighbor count within the same tick.
That guarantee already existed in `World`; `LifeSystem` just had to be simple enough to lean on it instead of re-implementing it.
`BoardSink` reacts to the same `engine.tick`/custom-event pair `GameStateReducer` already established, just aimed at `World` instead of a struct:

```cpp
if (event.event.name == antwika::engine::events::kTick)
{
    world.commit();
    scheduler.run(world, event.tick);
}
else if (event.event.name == events::kToggleCell)
{
    ...
    world.set<Cell>(entity, Cell{.alive = !wasAlive});
}
```

A `life.toggle_cell` event (payload `"x,y"`) seeds the initial pattern the same tick-stamped, replayable way `game.score_increment` does — `--record`/`--replay` reproduce the exact same run, because both apps go through the identical `EngineLoop`.

## Watching it happen, one generation at a time

The first version of `apps/life` only printed the final board.
A request to print every generation, not just the last one, led to two designs worth weighing against each other: give `EngineLoop` an `onTick` callback, or emit an event and let something react to it.
Both got a fair hearing, each with real trade-offs, and the deciding fact turned out to be a plan for *multiple, independent* observers down the line — a stats collector alongside a printer, say, neither aware of the other.

That framing exposed a third option hiding in plain sight.
An `ISystem` doesn't consume events at all — it already runs once per tick, already gets `World&`, and `SystemScheduler` already supports registering any number of independent systems into a phase.
`PrintSystem` is exactly that: a system that only reads `World` (via `world.view<Cell>()`), registered into a new `"observe"` phase created right after `"life"`:

```cpp
const auto lifePhase = scheduler.createPhase("life");
scheduler.addSystem(lifePhase, lifeSystem);

const auto observePhase = scheduler.createPhase("observe");
for (auto &observer : observers)
{
    scheduler.addSystem(observePhase, observer.get());
}
```

`bootstrap()` takes `std::vector<std::reference_wrapper<ISystem>> observers = {}`, so adding a second, unrelated observer later is exactly one line at the call site — no new event name, no dispatcher wiring, no change to `bootstrap()`'s signature.
A test proves the "independent" part isn't just a claim: a `CallCountingSystem` that does nothing but increment a counter runs alongside `PrintSystem` in the same phase, and neither affects the other's output.
This is also why the `EngineLoop::onTick` callback from the first attempt got reverted rather than kept alongside it — once phases already provide per-tick, independent, composable observation, keeping a second, event-free mechanism next to it would mean two ways to do the same thing for no remaining reason.

## Why the toggle events printed in the wrong order

Building this surfaced something that looked like a bug and wasn't one.
The terminal showed the board *before* the `life.toggle_cell` events that seeded it:

```
After tick 0:
.....
..#..
..#..
..#..
.....
Process event: life.toggle_cell
Process event: life.toggle_cell
Process event: life.toggle_cell
```

The toggles genuinely did happen first — `BoardSink` staged them into `World` the moment they were dispatched, before `engine.step` ever ran.
What printed late was the *log line about* them, not the effect.
Every dispatched event has two independent listeners with different timing: `TickedEventDispatcher` calls `ITimedEventSink`s (like `BoardSink`) synchronously, right at dispatch time, while `Engine::step` used to defer its own "Process event: ..." logging until a `while` loop at the very end of the step, after dispatching `engine.tick` had already triggered that tick's whole generation — commit, `LifeSystem`, and `PrintSystem` — synchronously, mid-dispatch.
Nothing was broken; the log was just a delayed transcript of things that had already finished happening.
That distinction — a queue drained only for logging, with the real work already done elsewhere — is what the rest of this post is about.

## Removing a log line, and the interface that came out with it

Two small requests followed: stop printing the initial board, and remove the `"Engine step: tick N"` log line, as its own commit.
Both landed cleanly.
The `Engine::step` log removal left this behind:

```cpp
void Engine::step(antwika::time::Tick tick)
{
    dispatcher.dispatch(Event{.name = events::kTick});

    while (!eventQueue.empty())
    {
        auto event = eventQueue.pop();
        logger.log(Level::Info, std::format("Process event: {}", event.name));
    }
}
```

That `while` loop was the natural next question: what's it actually for, now that the log line above it is gone?
Tracing it against the previous section's finding gave a clean answer — nothing ever reacted to an event by popping it off this queue.
Every real reaction (`BoardSink`, `GameStateReducer`, `EventRecorder`) already happened synchronously at dispatch time, through `IEventSink`/`ITimedEventSink`.
The loop's only remaining job was producing a log line per event, one at a time, after the fact.

The first fix proposed was to drop `IEventQueue` from `Engine` entirely, as the "clean" version of just deleting the loop.
That fix was wrong, and a direct question caught it before it shipped: *will the queue ever get empty, or is it ever growing?*
Removing `Engine`'s drain doesn't remove the growth — it relocates it.
`EventDispatcher::dispatch()` still calls `queue.enqueue(event)` on every single dispatched event, unconditionally, and a grep across the whole codebase confirmed `Engine::step`'s loop had been the *only* place, anywhere, that ever called `.pop()` on an `IEventQueue`.
Take that loop away without taking the enqueue away too, and the queue just grows, unbounded, for the life of any run — invisible in a four-tick demo, real in anything long-running.

The complete fix went the other direction: remove `IEventQueue` from `EventDispatcher`, not just from `Engine`.

```cpp
class EventDispatcher final : public IEventDispatcher
{
public:
    explicit EventDispatcher(
        std::vector<std::reference_wrapper<IEventSink>> sinks);

    void dispatch(Event event) override;

private:
    std::vector<std::reference_wrapper<IEventSink>> sinks;
};
```

That cascaded predictably: `EventDispatcher`'s constructor lost a parameter, both apps' `bootstrap()` signatures and every call site (`main.cpp`, `BootstrapTest`, `ReplayIntegrationTest`, in both `apps/game` and `apps/life`) dropped `eventQueue`, and `ReplayDeterminismTest` in `antwika::replay` needed the same update.
`IEventQueue`, `EventQueue`, `EventQueueTest`, and `MockEventQueue` were deleted outright rather than left in place with no caller — a producer with no consumer left isn't dead code to ignore, it's a live leak, and the same logic applies to the class that only ever fed one.
Deleting `EventQueueTest.cpp` had one more thing to say: it turned out to be the only file anywhere that even mentioned `MockEventRecorder.hpp`, via an `#include` the test never actually used.
`scripts/check_unused_test_doubles.py` caught it the moment that include disappeared, exposing a mock that had been dead code all along, just narrowly dodging the checker.
It got deleted too.

## Where it ended up

- `apps/life`: a Game of Life demo built entirely on `antwika::ecs` — `Cell`, `Grid`, `LifeSystem`, `BoardSink`, and `PrintSystem` — the first application to exercise `World`/`SystemScheduler`/phases end to end, with `--record`/`--replay` support identical to `apps/game`.
- `PrintSystem` as the template for independent per-tick observers: any number of systems can be registered into the `"observe"` phase, each unaware of the others, proven by a throwaway counting system sharing that phase in a test.
- Three commits: the app itself, the `Engine` log-line removal, and the `IEventQueue` removal that followed from asking what the log line's own draining loop was actually for.
- `IEventQueue`, `EventQueue`, and their test doubles deleted entirely — 25 files touched, 354 lines removed against 31 added, once every call site that only ever fed an unread queue was found and cleaned up.
- 140 tests passing at the end, 0 failures at any point along the way — including a `BootstrapTest` that asserts the exact printed transcript of a blinker oscillating for four generations.

The common thread is that "just delete this" was checked twice before it happened, not once — the log line came out cleanly the first time, and the queue underneath it only came out cleanly on the second attempt, after the first one was shown to just move the same problem somewhere quieter.
