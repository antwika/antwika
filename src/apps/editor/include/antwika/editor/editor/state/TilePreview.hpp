#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

#include <antwika/map/Layers.hpp>
#include <antwika/tilemap/Tilemap.hpp>

namespace antwika::editor
{

    struct TilePreview final
    {
        bool automatic = true;

        std::uint32_t seed = 0;

        std::optional<tilemap::Tile> forTile;

        std::size_t layer = map::kBaseLayer;

        std::optional<std::vector<std::optional<tilemap::Tile>>> tiles;
    };

}
