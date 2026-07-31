# antwika::pathfinding

`src/libs/pathfinding/` — A* over an abstract graph, with no grid inside it.

## What it is for

Finding a cheapest route between two nodes.
The world arrives through an `IGraph` supplying `neighbours()` and `heuristic()`, and no cell, coordinate or extent appears anywhere in the core.

`GridGraph` is a 4-connected convenience layered on top, for the callers that do happen to have a grid.

## Key types

| Header | Type | Role |
| --- | --- | --- |
| `NodeId.hpp` | `NodeId` | A scoped index; what a graph calls its nodes. |
| `Cost.hpp` | `Cost` | `std::int64_t` — integer throughout, so no float reaches a route. |
| `Neighbour.hpp` | `Neighbour` | A node and the cost of the edge reaching it. |
| `IGraph.hpp` | `IGraph` | `neighbours()` and `heuristic()`; the whole world, as far as the search is concerned. |
| `GridGraph.hpp` | `GridGraph`, `GridCell` | A 4-connected grid over a fixed extent, with per-cell passability. |
| `AStar.hpp` | `findPath()` | The search. |
| `SearchResult.hpp` | `SearchResult`, `SearchOutcome` | The nodes found, and whether a route existed at all. |
| `PathfindingError.hpp` | `PathfindingError` | For genuine precondition breaches, such as a negative edge cost. |

## Depends on

Nothing.

## Non-obvious decisions

**A missing path is an ordinary answer, not an exception.**
`SearchOutcome::NoPath` is what a walled-off goal produces, because being unable to get somewhere is a normal fact about a map rather than a failure of the search.
That leaves `PathfindingError` for things that are genuinely a caller's mistake.

**The open set's ordering is total, and that is the only reason a replay may depend on a path.**
It orders on estimated total cost, then remaining estimate, then ascending `NodeId`.
The third key is what makes it total: no two entries ever compare equivalent, so the heap never gets to choose, and an equal-cost route resolves the same way on every run and every toolchain.

Without that key the answer would still be *a* shortest path, and a different one on a different standard library — which is exactly the kind of divergence a replay cannot survive.

**A caller must keep its node numbering fixed.**
`GridGraph` numbers nodes row-major over the extent it is given, so the extent has to be a constant of the world rather than a bounding box derived from whatever is currently passable.
[game](../apps/game.md)'s `stepTowards()` passes the extent in for this reason: a box computed from the roads that happen to exist would renumber every node as one was laid, and with it the tie-break.

## See also

- [`docs/pathfinding.md`](../../docs/pathfinding.md) — the long-form argument.
- [game](../apps/game.md) — walks a walker home over the roads, re-searching every step.
