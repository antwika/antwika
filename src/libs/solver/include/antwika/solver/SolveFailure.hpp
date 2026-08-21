#pragma once

#include <cstdint>
#include <antwika/tilemap/AtlasLayout.hpp>
#include <antwika/tilemap/TileEdges.hpp>
#include <antwika/tilemap/Tilemap.hpp>
#include <antwika/tile/TileRules.hpp>
#include <antwika/voxel/VoxelCube.hpp>
#include <antwika/voxelmap/Voxel.hpp>

namespace antwika::solver
{

    enum class SolveFailure : std::uint8_t
    {
        None,

        EmptyDomain,

        IncompatibleEdge,

        Unsatisfiable,
    };

}
