#pragma once

#include <cstddef>

#include <antwika/tilemap/AtlasLayout.hpp>
#include <antwika/tilemap/TileEdges.hpp>
#include <antwika/tilemap/Tilemap.hpp>
#include <antwika/tile/TileRules.hpp>
#include <antwika/voxel/VoxelCube.hpp>
#include <antwika/voxelmap/Voxel.hpp>

namespace antwika::solver
{

    struct FaceSeam final
    {
        std::size_t faceA = 0;

        std::size_t faceB = 0;

        tilemap::TileEdge edgeA{};

        tilemap::TileEdge edgeB{};

        [[nodiscard]] bool operator==(const FaceSeam &other) const
            = default;
    };

}
