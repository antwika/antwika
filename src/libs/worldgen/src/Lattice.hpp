#pragma once

#include <array>
#include <cstddef>
#include <span>
#include <vector>

#include <antwika/wfc/Domain.hpp>
#include <antwika/worldgen/ruleset/CompiledRuleset.hpp>

#include "Board.hpp"
#include "Neighbour.hpp"
#include "antwika/worldgen/ChunkShape.hpp"
#include "antwika/worldgen/Ruleset.hpp"

namespace antwika::worldgen::detail
{

    [[nodiscard]] std::size_t neighboursOf(
        ChunkShape shape,
        std::size_t cell,
        std::array<Neighbour, 6> &foundNeighbours);

    [[nodiscard]] bool settle(
        const CompiledRuleset &compiledRuleset,
        ChunkShape shape,
        Board &board,
        const std::vector<std::size_t> &cells);

}
