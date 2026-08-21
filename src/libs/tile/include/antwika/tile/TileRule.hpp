#pragma once

#include <set>
#include <antwika/tilemap/TileEdges.hpp>
#include <antwika/tilemap/Tilemap.hpp>
#include <antwika/voxel/VoxelCube.hpp>

namespace antwika::tile
{

    struct TileRule final
    {
        tilemap::Tile tile{};

        tilemap::TileEdge edge{};

        std::set<tilemap::Tile> allowedTiles{};

        bool allowsBoundary = false;

        [[nodiscard]] bool operator==(const TileRule &other) const
            = default;
    };

}
