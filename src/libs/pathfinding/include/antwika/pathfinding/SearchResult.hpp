#pragma once

#include <cstdint>
#include <vector>

#include "antwika/pathfinding/Cost.hpp"
#include "antwika/pathfinding/NodeId.hpp"

namespace antwika::pathfinding
{

    enum class SearchOutcome : std::uint8_t
    {
        PathFound,
        NoPath,
    };

    struct SearchResult final
    {
        SearchOutcome outcome;
        std::vector<NodeId> nodes;
        Cost cost;
    };

}
