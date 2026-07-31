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

## See also

- [`docs/ecs-commons.md`](../../docs/ecs-commons.md) — the long-form argument.
