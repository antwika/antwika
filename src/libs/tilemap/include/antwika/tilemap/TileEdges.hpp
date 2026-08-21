#pragma once

#include <array>
#include <cstddef>

#include <antwika/voxel/VoxelCube.hpp>

namespace antwika::tilemap
{

    using TileEdge = voxel::FaceEdge;

    inline constexpr std::size_t kTileCorners = voxel::kFaceCorners;

    inline constexpr std::size_t kTileEdges = voxel::kFaceEdges;

    inline constexpr std::array<TileEdge, kTileEdges> kEveryTileEdge =
        voxel::kEveryFaceEdge;

}
