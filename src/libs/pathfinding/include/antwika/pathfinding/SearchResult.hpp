#pragma once

#include <vector>

#include "antwika/pathfinding/Cost.hpp"
#include "antwika/pathfinding/NodeId.hpp"

namespace antwika::pathfinding
{

    /**
     * @brief The two possible answers to findPath().
     *
     * Both are ordinary outcomes; neither is an error.
     */
    enum class SearchOutcome
    {
        PathFound,
        NoPath,
    };

    /**
     * @brief What one findPath() call worked out.
     *
     * `nodes` and `cost` mean nothing unless `outcome` is PathFound, in
     * which case `nodes` runs from the start to the goal inclusive --
     * so a search whose start already is its goal reports one node and
     * a cost of zero rather than an empty path.
     */
    struct SearchResult
    {
        SearchOutcome outcome;
        std::vector<NodeId> nodes;
        Cost cost;
    };

} // namespace antwika::pathfinding
