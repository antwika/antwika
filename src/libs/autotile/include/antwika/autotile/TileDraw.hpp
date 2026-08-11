#pragma once

#include <cstdint>
#include <vector>

#include <antwika/geometry/Point.hpp>
#include <antwika/tilemap/TerrainClass.hpp>

namespace antwika::autotile
{

    enum class DrawKind : std::uint8_t
    {
        Sprite = 0,
        WallRim,
        WallBand,
        BridgeDeck,
        Shade,
    };

    [[nodiscard]] constexpr DrawKind enumBound(DrawKind) noexcept
    {
        return DrawKind::Shade;
    }

    struct TileDraw final
    {
        tilemap::TerrainClass terrain = tilemap::TerrainClass::Floor;
        DrawKind kind = DrawKind::Sprite;

        /**
         * @brief The sprite's row in the bound tileset's atlas.
         *
         * Requires: read only when kind is DrawKind::Sprite.
         */
        std::uint16_t atlasRow = 0;

        /**
         * @brief The animation frame, resolved at plan time.
         *
         * Requires: read only when kind is DrawKind::Sprite.
         */
        std::uint8_t frame = 0;

        geometry::Point screen{};

        [[nodiscard]] bool operator==(const TileDraw &other) const
            = default;
    };

    using DrawPlan = std::vector<TileDraw>;

}
