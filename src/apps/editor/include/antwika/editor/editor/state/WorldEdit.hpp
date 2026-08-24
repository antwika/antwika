#pragma once

#include <cstdint>

#include <antwika/solver/VoxelWeave.hpp>

namespace antwika::editor
{

    struct WorldEdit final
    {
        std::int32_t editLevel = 0;

        solver::CornerSeams cornerJoining = solver::CornerSeams::Ignored;

        bool lowerSight = true;

        bool lowerLight = true;

        bool ascendHeld = false;

        bool descendHeld = false;
    };

}
