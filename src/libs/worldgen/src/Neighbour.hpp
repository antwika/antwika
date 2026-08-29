#pragma once

#include <cstddef>

#include <antwika/wfc/Domain.hpp>
#include <antwika/worldgen/ruleset/CompiledRuleset.hpp>

#include "antwika/worldgen/ChunkShape.hpp"
#include "antwika/worldgen/Ruleset.hpp"

namespace antwika::worldgen::detail
{

    struct Neighbour final
    {
        std::size_t otherCell = 0;

        Axis axis = Axis::Across;

        bool lower = true;
    };

}
