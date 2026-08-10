#pragma once

#include <cstdint>
#include <vector>

#include <antwika/geometry/Point.hpp>
#include <antwika/tilemap/TerrainClass.hpp>

#include "antwika/autotile/TilePiece.hpp"

namespace antwika::autotile
{

    struct TileDraw final
    {
        tilemap::TerrainClass terrain = tilemap::TerrainClass::Floor;
        TilePiece piece = TilePiece::Surface;
        std::uint8_t mask = 0;
        std::uint8_t variant = 0;
        geometry::Point screen{};

        [[nodiscard]] bool operator==(const TileDraw &other) const
            = default;
    };

    using DrawPlan = std::vector<TileDraw>;

}
