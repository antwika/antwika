# antwika::ecs

`src/libs/ecs/` — entities, components, and systems with nowhere to hide a mutation.

## What it is for

Holding application state as entities carrying components, advanced once per tick by systems.
It is one of the two ways an app can hold state in this project; the other is a plain value folded by an `ITickEventSink` the app owns.

## Key types

| Header | Type | Role |
| --- | --- | --- |
| `Entity.hpp` | `Entity` | An `enum class : std::uint64_t` identifier, not a pointer or an index into anything a caller sees. |
| `Component.hpp` | — | The component concept a stored type must satisfy. |
| `ComponentStorage.hpp` | `ComponentStorage` | Per-type dense storage. |
| `World.hpp` | `World` | `create()`, `destroy()`, `alive()`, `add<T>()`, `remove<T>()`, `has<T>()`, `get<T>()`, `set<T>()`, `view<Ts...>()` and `commit()`. |
| `View.hpp` | `View` | Iterating the entities that carry a given component set. |
| `ISystem.hpp` | `ISystem` | `update(World &, time::Tick)`. |
| `SystemScheduler.hpp` | `SystemScheduler` | Runs registered systems, ordered by `Phase`. |
| `Phase.hpp` | `Phase`, `PhaseId` | Named ordering slots for systems. |
| `EcsError.hpp` | `EcsError` | Touching an entity that is not alive, and similar misuse. |

`MockSystem` lives under `tests/mocks/`.

## Depends on

[`log`](log.md), [`time`](time.md).

## Non-obvious decisions

**The world is double-buffered and mutations are staged.**
`add()` and `remove()` push a pending operation rather than editing storage immediately; the change becomes visible at the commit between ticks.
This is the point of the library: a system reads the committed board, so two systems in one tick cannot see each other's half-finished work, and the order they happen to run in cannot change the result.
It is also why `apps/life`'s `PointerToggleSink` keeps its own note of what the current tick has staged — the `World` will hand it the committed board, which would let two drags over one cell in a tick collapse into a single toggle.

**A dead entity is an error, not a silent no-op.**
`add()` and `remove()` throw `EcsError` if the entity is not alive, which is what stops a zombie component from quietly reappearing after a `destroy()`.

**`add()` on a component the entity already has overwrites it.**
It is a second deferred write path beside `set()`, and the two are ordered by mechanism rather than by call order: an `add()` lands *during* `commit()`, so it beats a same-phase `set()` of the same component whichever was called first, and two `add()`s in one phase resolve as the later one.
A caller meaning "only if it is not there" has to ask `has<T>()` itself, as [game](../apps/game.md)'s `ProductionSystem` and `MarketSystem` do — and `has<T>()` answers as of the last commit, so two systems in one phase can both be told no.
`WorldTest` pins all four of those.

**An entity index is never reused, and each pool's sparse array grows with the highest one it ever saw.**
`EntityManager` hands out monotonically increasing values with no free list and no generation counter, because the only thing a generation counter guards against is a stale handle aliasing a recycled index, and nothing can alias what is never recycled.
The standing cost is memory: a `ComponentStorage`'s sparse array is indexed by raw entity value and never shrinks, so a pool costs O(highest entity value ever inserted into it) rather than O(the entities it currently holds), and a long session churning short-lived entities grows every pool one of them touched.
It is a `std::size_t` per index rather than a component, and nothing in the tree has measured it as a problem; the escape hatch when something does is a paged sparse index — fixed-size pages allocated on first use — which keeps the lookup O(1) for one extra indirection, rather than recycling indices, which is the decision already made the other way.

**A system stopping does not stop the tick.**
`apps/life`'s `DragPausedSystem` wraps `LifeSystem` and stages nothing while the pointer button is held, but the tick, the commit and every observing system still run — which is exactly why the cells being painted show up immediately.

See [`blog/003-an-entity-component-system-with-nowhere-to-hide-a-mutation.md`](../../blog/003-an-entity-component-system-with-nowhere-to-hide-a-mutation.md).
