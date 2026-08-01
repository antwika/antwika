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

**The graph is an interface, not a data structure.**
`IGraph` asks two questions — `neighbours()` and `heuristic()` — and there is no node table, no extent, no coordinate and no cell type anywhere in the library.
A `NodeId` is an opaque number the caller assigns, so a flat grid index, a room number and a handle into somebody else's table all reach the same search unchanged.
That matters because a library that grew a `Cell` type to serve [game](../apps/game.md) would have to grow a second one for the next caller.
`GridGraph` is therefore layered *on* the search rather than built into it: one implementation of `IGraph` with no privileges the search knows about, and deleting it would leave the algorithm untouched.

`neighbours()` appends to a vector the search owns and clears, rather than returning one.
Expanding a node is the inner loop of the whole search, and it is the one place in the interface where an allocation per call would be felt.

**A missing path is an ordinary answer, not an exception.**
`SearchOutcome::NoPath` is what a walled-off goal produces, because being unable to get somewhere is a normal fact about a map rather than a failure of the search.
That leaves `PathfindingError` for things that are genuinely a caller's mistake.

**The open set's ordering is total, and that is the only reason a replay may depend on a path.**
It orders on estimated total cost, then remaining estimate, then ascending `NodeId`.
The third key is what makes it total: no two entries ever compare equivalent, so the heap never gets to choose, and an equal-cost route resolves the same way on every run and every toolchain.

Without that key the answer would still be *a* shortest path, and a different one on a different standard library — which is exactly the kind of divergence a replay cannot survive.
Two routes of equal cost are common on a grid; the trivial 2x2 case already has two.
The second key earns its place too: preferring the entry nearer the goal among equals drives the search forwards rather than sideways.

One more rule completes it: where two routes reach a node for the same cost, the first found under that order is kept and a later tie never displaces it.
`Cost` is an exact `std::int64_t` for the same reason — every one of those tie-breaks rests on two costs comparing equal, and "equal" has to mean the same thing on all three toolchains, which a rounded floating-point sum cannot promise.
On a `GridGraph`, ascending `NodeId` is row-major order, so a tie resolves upwards and leftwards.

**The heuristic must be consistent, not merely admissible.**
A node is closed once and never reopened, which is the usual A* trade and keeps the search linear in what it touches.
The price is that `h(a) <= cost(a -> b) + h(b)` has to hold.
Hand an inconsistent estimate to the search and it still terminates and still returns a path — it may just cost more than the cheapest one, and `AStarHeuristicTest` pins that behaviour down rather than leaving it folklore.
`GridGraph`'s Manhattan distance is consistent by construction, since one step changes it by exactly one and every step costs the same; returning `0` is always consistent too, and turns the search into Dijkstra's.

**A caller must keep its node numbering fixed.**
`GridGraph` numbers nodes row-major over the extent it is given, so the extent has to be a constant of the world rather than a bounding box derived from whatever is currently passable.
[game](../apps/game.md)'s two callers both pass the extent in for this reason: a box computed from the roads that happen to exist would renumber every node as one was laid, and with it the tie-break.
`stepTowards()` is the one that walks a walker home, re-searching every step and using only the first move; `planRoad()` is the one that turns a road drag's two ends into the run of cells laid between them, and it consults no roads at all, since an existing one is passable and simply not laid again.

## See also

- [game](../apps/game.md) — walks a walker home over the roads, re-searching every step, and plans the run of road a drag lays between its two ends.
- [`wfc`](wfc.md) — the same bargain: the library owns an algorithm, the caller owns the geometry.
