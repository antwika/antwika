# A commons library for shared ECS content

Status: current.
This document records why `antwika::ecs_commons` exists as its own library rather than as more headers inside `antwika::ecs`, and what went into it.

## The question

Three apps build on `antwika::ecs`, and each of them has had to write its own components and its own per-tick systems.
The question was whether the components and systems that recur belong in `antwika::ecs` itself, or in a separate library beside it.

## The decision

A separate library, `antwika::ecs_commons`, depending on `antwika::ecs` and `antwika::time` and nothing else.

`antwika::ecs` is a *mechanism*.
`World`, `ComponentStorage`, `View` and `SystemScheduler` describe how state is held and when it changes, and they say nothing whatsoever about what the state is -- the double-buffered commit works identically for a Conway cell, a poker seat and a walker.
That is what lets the blog post about it claim there is nowhere to hide a mutation: the claim is about the mechanism, and it survives contact with any content at all.

A `Lifetime` component and a `LifetimeSystem` are *content*.
They are one answer to one modelling question, and an app is free to disagree with them.
Folding them into `antwika::ecs` would mean every app that wants a scheduler also links a countdown it does not use, and -- worse -- that a change to what "expiring" means is a change to the library every ECS app depends on.
The repo's stated layering premise is small, single-purpose libraries, and "the ECS mechanism" and "a vocabulary of common game components" are two purposes.

The split also keeps the dependency arrow honest.
`ecs_commons` depends on `antwika::time` for `Tick`, which `antwika::ecs` already does too, but a future commons component wanting `antwika::gfx` or `antwika::input` would be a straightforward addition here and an unacceptable one in `antwika::ecs`.

### The counter-argument, which is real

Two arguments point the other way, and neither is silly.

The first is simply that this is one more library, one more `CMakeLists.txt`, one more entry in `src/libs/CMakeLists.txt`, for a handful of structs.
That cost is real and it is paid once.

The second is better: a shared vocabulary is arguably part of what an ECS *is* for.
Most published ECS frameworks ship a `Transform` and a `Parent` in the core, precisely because interoperability between two systems written by two people depends on them agreeing on the type.
An app that defines its own `Position` cannot use a commons `MovementSystem`, so the vocabulary only pays off if everyone adopts it -- and putting it in the core is the strongest way of making that happen.

That argument loses here for a specific reason rather than a general one: this repo has no third-party consumers to interoperate with, so the "everyone adopts it" pressure the core placement buys is pressure it does not need.
What it does have is a hard rule that every line of `src/` is covered and every dependency is justified, which makes unused content in a widely-linked library expensive in a way it is not elsewhere.
If that ever changes -- if `antwika::ecs` is published on its own -- promoting `GridPosition` into it is a move, not a rewrite.

## What is actually duplicated today

This is the survey the library was populated from, not a generic ECS wishlist.

- `game::Cell` (`src/apps/game/include/antwika/game/Cell.hpp`) is `std::int32_t x, y` with `==` and `<=>`, ordered so it can key a `std::map`.
  `ecs_commons::GridPosition` is near-identical to it, down to the reason for the ordering.
- `life::Grid` (`src/apps/life/include/antwika/life/Grid.hpp`) maps `(x, y)` to an `Entity`, and those coordinates are the same idea in `std::uint32_t`.
  `GridPosition` would be the component, with the grid index staying app-side.
- `task_worker::Worker::label` and `makeWorkerLabel()` (`src/apps/task_worker/include/antwika/task_worker/Worker.hpp`) are a fixed `std::array<char, 32>` with a truncating builder, written that way because `ecs::Component` forbids `std::string`.
  `ecs_commons::Name`, `makeName()` and `view()` are that same workaround, written once.
- `task_worker::Worker::remainingTicks` and `WorkerCompletionSystem` (`src/apps/task_worker/src/WorkerCompletionSystem.cpp`) count a `Tick` down and act at zero.
  `Lifetime` and `LifetimeSystem` are the countdown half, with the app keeping the "what happens at zero" half.
- `game::WalkerSystem` (`src/apps/game/src/WalkerSystem.cpp`) steps every walker one cell every `game::kTicksPerStep` ticks, staging into the back buffer.
  `Velocity` and `MovementSystem` are the integration half, with the pathfinding staying app-side.
  Note that the cadence is *not* the half `PeriodicSystem` covers -- see migration 5 below for why.
- `game::Path` (`src/apps/game/include/antwika/game/Path.hpp`) is an empty struct used purely as a marker, which is exactly `ecs_commons::Tag<Kind>`.
- `life::DragPausedSystem` (`src/apps/life/src/DragPausedSystem.cpp`) is an `ISystem` that wraps another and conditionally declines to run it.
  `PeriodicSystem` is the same decorator shape, with the condition being the tick number.

## What went in

Eight pieces, in `src/libs/ecs_commons/include/antwika/ecs_commons/`.

- `GridPosition` -- signed integer cell coordinates, `==` and `<=>`.
- `Velocity` -- signed integer cells per tick, plus the pure `stepBy()` that applies one to a position.
- `MovementSystem` -- integrates `Velocity` into `GridPosition` once per tick.
- `Lifetime` -- how many ticks an entity has left.
- `LifetimeSystem` -- counts it down and destroys the entity at zero.
- `Name` -- a fixed-buffer label, with `makeName()` and `view()`.
- `Tag<Kind>` -- a stateless marker component, distinct per `Kind`.
- `PeriodicSystem` -- an `ISystem` decorator that runs another system every nth tick.
- `EcsCommonsError` -- the one exception type here, thrown for a cadence of zero.

Everything is integers.
Nothing in simulation state is floating point, for the reason the rest of the repo gives: a replay reproduces state by re-running the same arithmetic, and that only holds when the arithmetic is exact.
`stepBy()` narrows through `std::uint32_t` so an overflowing step wraps identically everywhere rather than being undefined.

Both systems read the front buffer and stage into the back, exactly as `life::LifeSystem` and `game::WalkerSystem` do, so nothing within a tick can see another entity's half-applied move.
Both iterate a `View`, whose order is `ComponentStorage`'s insertion order -- no unordered container is ever iterated to produce state.

### What was deliberately left out

A `Health` component and a `DamageSystem` were considered and are not here.
No app in the repo has hit points, or anything resembling them, so there is no duplication to remove and no second caller to check the design against.
`game::WalkerSystem`'s own comment gives the rule this follows: inventing a rule to avoid a situation nothing requires would be inventing a requirement.
When a second app wants damage, the shape it wants will be visible, and adding it then costs a header.

A `Parent`/`Transform` hierarchy was also left out.
Nothing composes entities today, and a hierarchy has a commit-ordering question (does a child see its parent's move this tick or next?) that should be answered by a real caller rather than guessed at.

## Follow-up: migrating the apps

Nothing was refactored as part of adding this library, because several apps were being edited concurrently.
The migrations that look worthwhile, in rough order of value:

1. `task_worker::Worker::label` / `makeWorkerLabel()` -> `ecs_commons::Name` / `makeName()`.
   The truncation behaviour matches, but the buffer is **not** the same size and this is **not** a straight substitution.
   `Worker::label` is `std::array<char, kWorkerLabelMaxLength + 1>` and is always null-terminated; `Name::text` is `std::array<char, kNameMaxLength>` with no terminator slot at all.
   `StatusPrintSystem.cpp` streams `worker.label.data()`, which reads until a NUL, so substituting the type without also changing that call site to `view(worker.name)` reads past the end of the array for any label of exactly 31 characters.
   Migrate the call site and the type together, or not at all.
2. `game::Cell` -> `ecs_commons::GridPosition`.
   Mechanically a rename plus a member rename, but it touches `IsoProjection`, `PathIndex`, `GridSink`, `SceneSnapshot` and their tests, so it is the largest of these.
3. `game::Path` -> `using Path = ecs_commons::Tag<struct PathKind>;`.
   One line, once `game::Cell` has moved or independently of it.
4. `task_worker::WorkerCompletionSystem` -> keep the system, but hold the countdown in a `Lifetime` and let `LifetimeSystem` do the decrement, leaving the completion system to react to expiry.
   This one needs thought: today the system also reports progress to `TaskRegistry` every tick, so the split is not free.
5. `game::WalkerSystem` -> `MovementSystem` is *not* a drop-in, because a walker's step comes from `nextFacing()` rather than a stored velocity.
   The plausible shape is for `WalkerSystem` to write a `Velocity` and let `MovementSystem` integrate it.
   What that shape does **not** buy is the cadence: a walker already moves once every `game::kTicksPerStep` ticks, counted down in its own `Walker` component rather than off the tick number, and neither half of this library can express that.
   `Velocity` is whole cells per tick with no notion of a slower one, and `PeriodicSystem` gates a whole system on `tick % period`, which is the tick number -- so composing the two would move a per-entity countdown back onto the global clock and make every walker share one speed by construction.
   A commons answer would be a per-entity cadence component, which no second caller has asked for yet.

`life` is the odd one out: its cells never move, never expire and have no label, so it has nothing to migrate beyond possibly keying its `Grid` on `GridPosition`.
