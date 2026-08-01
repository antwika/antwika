#include "antwika/pathfinding/AStar.hpp"

#include <algorithm>
#include <limits>
#include <map>
#include <queue>
#include <set>
#include <vector>

#include "antwika/pathfinding/Cost.hpp"
#include "antwika/pathfinding/Neighbour.hpp"
#include "antwika/pathfinding/PathfindingError.hpp"

namespace antwika::pathfinding
{

    namespace
    {

        struct OpenEntry
        {
            Cost estimate;
            Cost remaining;
            NodeId node;
            Cost cost;
        };

        // The whole determinism claim lives here.
        // Order: estimated total, then remaining estimate, then id.
        // std::priority_queue pops the greatest, so this says "worse".
        // The third key stops two nodes ever comparing equivalent.
        struct WorseFirst final
        {
            [[nodiscard]] bool operator()(
                const OpenEntry &left, const OpenEntry &right) const noexcept
            {
                if (left.estimate != right.estimate)
                {
                    return left.estimate > right.estimate;
                }

                if (left.remaining != right.remaining)
                {
                    return left.remaining > right.remaining;
                }

                return rawValue(left.node) > rawValue(right.node);
            }
        };

        struct Visit final
        {
            Cost cost;
            NodeId parent;
        };

        using OpenSet =
            std::priority_queue<OpenEntry, std::vector<OpenEntry>, WorseFirst>;

        [[nodiscard]] Cost estimateFor(
            const IGraph &graph, NodeId from, NodeId goal)
        {
            const Cost remaining = graph.heuristic(from, goal);

            if (remaining < 0)
            {
                throw PathfindingError(
                    "pathfinding: heuristic returned a negative estimate");
            }

            return remaining;
        }

        // Cost is signed, so a sum past its range is undefined.
        // Refusing rather than saturating is the deliberate choice here.
        // Two saturated keys compare equal, which reorders the open set.
        // That order is the whole of this library's determinism claim.
        // A cost that overflows is a broken precondition, not a missing path.
        // Both arguments have already been checked non-negative.
        // So the guard is a subtraction, which cannot itself overflow.
        [[nodiscard]] Cost addCosts(Cost left, Cost right)
        {
            constexpr Cost kMaxCost = std::numeric_limits<Cost>::max();

            if (left > kMaxCost - right)
            {
                throw PathfindingError(
                    "pathfinding: costs sum past what a Cost can hold");
            }

            return left + right;
        }

        // find() rather than at() here.
        // Every node on the chain was put there by this same search.
        [[nodiscard]] std::vector<NodeId> chainTo(
            const std::map<NodeId, Visit> &visited, NodeId start, NodeId goal)
        {
            std::vector<NodeId> nodes;
            NodeId node = goal;

            while (node != start)
            {
                nodes.push_back(node);
                node = visited.find(node)->second.parent;
            }

            nodes.push_back(start);
            std::reverse(nodes.begin(), nodes.end());

            return nodes;
            // The brace below is a cleanup only bad_alloc reaches.
            // wfc's extractAssignment excludes the same artefact.
        } // GCOVR_EXCL_LINE

    } // namespace

    SearchResult findPath(const IGraph &graph, NodeId start, NodeId goal)
    {
        const Cost startEstimate = estimateFor(graph, start, goal);

        std::map<NodeId, Visit> visited;
        visited.emplace(start, Visit{.cost = 0, .parent = start});

        OpenSet open;
        open.push(OpenEntry{
            .estimate = startEstimate,
            .remaining = startEstimate,
            .node = start,
            .cost = 0,
        });

        std::set<NodeId> closed;
        std::vector<Neighbour> neighbours;

        while (!open.empty())
        {
            const OpenEntry current = open.top();
            open.pop();

            // A cheaper route is queued, never swapped into place.
            // So the entry it replaced is still in the heap.
            if (!closed.insert(current.node).second)
            {
                continue;
            }

            if (current.node == goal)
            {
                return {
                    SearchOutcome::PathFound,
                    chainTo(visited, start, goal),
                    current.cost,
                };
            }

            neighbours.clear();
            graph.neighbours(current.node, neighbours);

            for (const Neighbour &neighbour : neighbours)
            {
                if (neighbour.cost < 0)
                {
                    throw PathfindingError(
                        "pathfinding: graph reported a negative edge cost");
                }

                if (closed.contains(neighbour.node))
                {
                    continue;
                }

                const Cost cost = addCosts(current.cost, neighbour.cost);
                const auto seen = visited.find(neighbour.node);

                if (seen != visited.end() && seen->second.cost <= cost)
                {
                    continue;
                }

                const Cost remaining =
                    estimateFor(graph, neighbour.node, goal);
                const Cost estimate = addCosts(cost, remaining);

                visited.insert_or_assign(
                    neighbour.node,
                    Visit{.cost = cost, .parent = current.node});

                open.push(OpenEntry{
                    .estimate = estimate,
                    .remaining = remaining,
                    .node = neighbour.node,
                    .cost = cost,
                });
            }
        }

        return SearchResult{
            .outcome = SearchOutcome::NoPath,
            .nodes = {},
            .cost = 0,
        };
    }

} // namespace antwika::pathfinding
