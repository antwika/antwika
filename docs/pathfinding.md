# Pathfinding

`antwika::pathfinding` is an A* search with no idea what a world looks like.
It is the same bargain [`antwika::wfc`](../src/libs/wfc) makes: the library owns an algorithm, the caller owns the geometry, and the two meet at one small interface.
This note records the two decisions a reader is most likely to want justified -- why the graph arrives through `IGraph` rather than as a grid, and what breaks a tie in the open set.

## The graph is an interface, not a data structure

`pathfinding::IGraph` asks two questions and nothing else:

```cpp
virtual void neighbours(NodeId from, std::vector<Neighbour> &out) const = 0;
[[nodiscard]] virtual Cost heuristic(NodeId from, NodeId goal) const = 0;
```

There is no node table, no extent, no coordinate and no cell type anywhere in the library.
A `NodeId` is an opaque number the caller assigns, so a flat grid index, a room number and a handle into somebody else's table all reach the same search unchanged.

That matters because `apps/game` is not the only shape a route could ever be wanted on, and a library that grew a `Cell` type to serve it would have to grow a second one for the next caller.
The 4-connected grid every tile-based caller actually wants is therefore `GridGraph`, a separate header *layered on* the search rather than built into it: it is one implementation of `IGraph`, with no privileges the search knows about, and deleting it would leave the algorithm untouched.

`neighbours()` appends to a vector the search owns and clears, rather than returning one.
Expanding a node is the inner loop of the whole search, and this is the one place in the interface where an allocation per call would be felt.

## Ties break on a stated rule, because a replay depends on it

A tick-loop app is only reproducible if every function inside the tick is.
Two routes of equal cost are common on a grid -- the trivial 2x2 case already has two -- so "the cheapest path" does not name a single answer, and a search that let the heap decide would be free to answer differently on another standard library, another optimisation level, or another day.

So the open set is ordered on three keys, in this order:

1. **Lower estimated total cost** (`g + h`) first, which is what makes it A* at all.
2. **Lower remaining estimate** (`h`) next, which prefers the entry nearer the goal among equals, and so drives the search forwards rather than sideways.
3. **Lower `NodeId`** last.

The third key is not a nicety, it is what makes the order *total*: no two entries in the open set name the same node at the same cost, so no two entries ever compare equivalent, and the heap is never asked to choose.
Nothing in the result therefore depends on the order edges were declared in, on how a heap arranged equal keys, or on any address.

The rule reaches back out to the caller, which is why `NodeId`'s ordering is documented as load-bearing rather than incidental: renumbering the nodes may return a different, equally cheap path.
On a `GridGraph`, ascending `NodeId` is row-major order, so a tie resolves upwards and leftwards.

One more rule completes it: where two routes reach a node for the same cost, the first one found under that order is kept, and a later tie never displaces it.

`Cost` is an exact `std::int64_t` for the same reason.
Every one of those tie-breaks rests on two costs comparing equal, and "equal" has to mean the same thing on all three toolchains, which a rounded floating-point sum cannot promise.

## No path is an answer, not an error

`findPath()` returns a `SearchResult` whose `outcome` is `PathFound` or `NoPath`.
A walker asked to reach a cell it has been walled away from is an ordinary situation an app handles, not an exceptional one, so it is not worth an exception and would be tedious to catch.

`PathfindingError` exists for the genuinely different case: a caller that has broken the algorithm's preconditions.
That is a negative edge cost or a negative heuristic reported during a search, and a `GridGraph` built from a non-positive extent, given the wrong number of passability flags, or asked about a cell outside its own bounds.
No amount of searching recovers from any of those, which is what separates them from `NoPath`.

## Consistency, not just admissibility

A node is closed once and never reopened, which is the usual A* trade and keeps the search linear in what it touches.
The price is that the heuristic has to be *consistent* (`h(a) <= cost(a -> b) + h(b)`), not merely admissible.
Hand an inconsistent estimate to the search and it still terminates and still returns a path -- it may just be a path that costs more than the cheapest one, and `AStarHeuristicTest` pins that behaviour down rather than leaving it folklore.

`GridGraph`'s Manhattan distance is consistent by construction, since one step changes it by exactly one and every step costs the same.
Returning `0` is always consistent too, and turns the search into Dijkstra's.
