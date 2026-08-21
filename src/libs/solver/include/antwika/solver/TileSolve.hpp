#pragma once

#include <cstddef>
#include <optional>
#include <vector>
#include <antwika/tilemap/AtlasLayout.hpp>
#include <antwika/tilemap/TileEdges.hpp>
#include <antwika/tilemap/Tilemap.hpp>
#include <antwika/tile/TileRules.hpp>
#include <antwika/voxel/VoxelCube.hpp>
#include <antwika/voxelmap/Voxel.hpp>
#include "antwika/solver/SolveFailure.hpp"

namespace antwika::solver
{

    struct TileSolve final
    {
        std::optional<std::vector<tilemap::Tile>> tiles{};

        SolveFailure troubleFailure = SolveFailure::None;

        tilemap::Atlas unsatisfiedAtlas = tilemap::Atlas::Wall;

        tilemap::TileEdge unsatisfiedEdge{};

        std::size_t skippedFaceCount = 0;

        std::vector<voxelmap::FaceRef> conflictFaces{};
    };

}
