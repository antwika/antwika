#pragma once

#include <antwika/tilemap/AtlasLayout.hpp>
#include <antwika/tilemap/TileEdges.hpp>
#include <antwika/tilemap/Tilemap.hpp>
#include <antwika/tile/TileRules.hpp>
#include <antwika/voxel/VoxelCube.hpp>
#include <antwika/voxelmap/Voxel.hpp>
#include "antwika/solver/SolveFailure.hpp"

namespace antwika::solver
{

    struct WeaveGap final
    {
        SolveFailure troubleFailure = SolveFailure::None;

        tilemap::Atlas unsatisfiedAtlas = tilemap::Atlas::Wall;

        tilemap::TileEdge unsatisfiedEdge{};

        [[nodiscard]] bool operator==(const WeaveGap &other) const
            = default;
    };

}
