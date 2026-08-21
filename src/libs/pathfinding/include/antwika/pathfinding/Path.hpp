#pragma once

#include <cstdint>
#include <optional>
#include <vector>

#include "antwika/pathfinding/IWalkGraph.hpp"
#include "antwika/pathfinding/GridPos.hpp"

namespace antwika::pathfinding
{

    [[nodiscard]] std::optional<std::vector<GridPos>> pathBetween(
        const IWalkGraph &graph,
        GridPos fromPosition,
        GridPos toPosition,
        std::uint64_t maxNodesExplored);

}
