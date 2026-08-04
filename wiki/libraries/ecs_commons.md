# antwika::ecs_commons

`src/libs/ecs_commons/` — the vocabulary half of the ECS: what an app names an entity with, and the gate it runs a system behind.

## What it is for

Two things, both header-only.

`Name` is a component holding a short label, for an entity a diagnostic or a readout has to talk about.
`GatedSystem` is the decorator that runs a system only while something else says it may -- **staging nothing is what holds a world still**, so the commit after that phase finds only what the tick's input did.

It is separate from [`ecs`](ecs.md) so that an app wanting the scheduler does not link vocabulary it never uses.

## Key types

| Header | Type | Role |
| --- | --- | --- |
| `Name.hpp` | `Name` | A short label on an entity. |
| `GatedSystem.hpp` | `GatedSystem` | Runs an inner `ISystem` only on ticks a predicate allows. |

## Depends on

[`ecs`](ecs.md) and [`time`](time.md), for `ISystem` and `Tick`.

## Non-obvious decisions

**`GatedSystem` takes a predicate rather than naming a condition**, because three applications had written the same class out with three different questions in the middle of it: `apps/game`'s `PauseGatedSystem` and `SessionGatedSystem`, and `apps/life`'s `DragPausedSystem`.
Each of those still exists and still has its own name -- "this system is pause-gated" is worth saying at the registration site -- but each is now a forwarder, so the mechanism is written once.
The predicate is asked once per tick rather than once at construction, so a gate that opens mid-run opens for the very next tick.

**This library used to be much larger, and the rest was pruned.**
`GridPosition`, `Velocity`, `Lifetime`, `Tag<Kind>`, `MovementSystem`, `LifetimeSystem`, `PeriodicSystem` and the error type only `PeriodicSystem` threw were all offered here and **never called by any application in the tree**.
They were tested to 100%, which is exactly why the gap was invisible: the coverage gate says nothing about a surface nobody uses.
A library is not a place to keep code warm on the chance somebody wants it -- git remembers it, and a reader of this page should not have to work out which half is real.

That removed the file's other oddity with them: the sources used to be compiled *into* the test binary rather than linked, to stop the linker discarding a second copy of the `ecs` templates and zeroing their counters.
With nothing left to compile, the arrangement went too.

**Adopting `Name` is a real thing an app can do and mostly has not.**
Twelve files use it; `apps/game` still has its own `Cell` where a `GridPosition` would have done.
That migration was considered and is not obviously worth its blast radius -- `IsoProjection`, `PathIndex`, `GridSink` and `SceneSnapshot` all speak `Cell` -- which is part of why the unused half was pruned rather than kept waiting for a caller.
