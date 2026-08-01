# antwika::ecs_commons

`src/libs/ecs_commons/` — the vocabulary half of the ECS.

## What it is for

The components and systems that most ECS applications want, kept **out** of [`ecs`](ecs.md) itself.

`ecs` is the mechanism — entities, storages, the double-buffered `World`, the scheduler.
These are content: a position, a velocity, a countdown, and the systems that act on them.
Splitting them means an application that wants the scheduler does not link a countdown it never uses.

## Key types

| Header | Type | Role |
| --- | --- | --- |
| `GridPosition.hpp` | `GridPosition` | Integer cell coordinates. |
| `Velocity.hpp` | `Velocity` | Integer per-tick movement. |
| `Lifetime.hpp` | `Lifetime` | A countdown in ticks. |
| `Name.hpp` | `Name` | A fixed-capacity name, so it stays a `Component`. |
| `Tag.hpp` | `Tag<Kind>` | An empty marker component, one type per kind. |
| `MovementSystem.hpp` | `MovementSystem` | Applies `Velocity` to `GridPosition`. |
| `LifetimeSystem.hpp` | `LifetimeSystem` | Counts `Lifetime` down and destroys at zero. |
| `PeriodicSystem.hpp` | `PeriodicSystem` | Runs another system every N ticks. |
| `EcsCommonsError.hpp` | `EcsCommonsError` | This library's one failure type. |

## Depends on

[`ecs`](ecs.md) and [`time`](time.md).

## Non-obvious decisions

**Everything here is integer.**
A component that a replay reproduces may not hold floating point, so `Velocity` moves whole cells per tick rather than a fraction of one.
Sub-tick smoothness is a rendering question, and [`animation`](animation.md) is where it is answered.

**`Name` has a fixed capacity rather than holding a `std::string`.**
`ecs`'s `Component` concept requires trivially copyable and standard layout, which a `std::string` is not.
The cap is the price of a name being storable in a component at all.

**`Tag<Kind>` is a template so that two tags are two types.**
One `Tag` type with a runtime kind field would make "every entity tagged X" a filter rather than a view, and the whole point of a tag is that the storage already knows.

**Not every app should use these.**
[game](../apps/game.md) deliberately does not link this library: its walkers count steps rather than ticks, so `Lifetime` would be the wrong shape, and it would have become the only game-side caller of the whole module for one integer.
Content is worth sharing only when it is genuinely the same content.

**A `Health` component and a `Parent`/`Transform` hierarchy were considered and left out.**
No app here has hit points or composes entities, so there is no duplication to remove and no second caller to check a design against — and a hierarchy additionally has a commit-ordering question (does a child see its parent's move this tick or next?) that a real caller should answer rather than a guess.
Inventing a rule to avoid a situation nothing requires would be inventing a requirement.

## Migrating an app onto these

Nothing was refactored when this library was added, because several apps were being edited concurrently.
Two of the candidates are worth doing and two should be struck rather than scheduled.

- **`game::Cell` → `GridPosition`, and `game::Path` → `Tag<struct PathKind>`.**
  Both are drop-ins: field-for-field identical, ordering and all, and both tags are empty structs with a defaulted `==`.
  Mechanical; only the blast radius is large, since the first touches `IsoProjection`, `PathIndex`, `GridSink`, `SceneSnapshot` and their tests.
- **`task_worker::Worker::label` → `Name`** is a drop-in for the type and *not* for the call sites.
  `Worker::label` is always null-terminated; `Name::text` has no terminator slot at all, and `StatusPrintSystem.cpp` streams `label.data()`, which reads until a NUL.
  Substituting the type without changing that call site to `view(worker.name)` reads past the end of the array for a label of exactly the maximum length.
  Migrate the call site and the type together, or not at all.
- **`task_worker`'s completion countdown must *not* become a `Lifetime`.**
  `LifetimeSystem` destroys the entity at zero, and this app's workers are a fixed pool created once — a finished worker goes Idle and waits.
  The substitution would delete one worker per completed task, and there is no "react to expiry" hook, because here expiry *is* the destruction.
- **`game::WalkerSystem` is not a `MovementSystem`.**
  A walker's step comes from `nextFacing()` rather than a stored velocity, and more importantly neither half of this library can express its cadence: `Velocity` is whole cells per tick with no notion of a slower one, and `PeriodicSystem` gates on `tick % period`, which is the global clock.
  Composing them would move a per-entity countdown back onto the global tick and make every walker share one speed by construction.

The two components a migration would want and not find — a `Lifetime` that expires without destroying, and a per-entity cadence — are each wanted by exactly one caller today, which is the same bar `Health` and `Parent` failed.
