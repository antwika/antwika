#pragma once

#include <optional>

#include <antwika/geometry/Grid.hpp>
#include <antwika/gfx/PointF.hpp>
#include <antwika/gfx/RectF.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/tilemap/Tilemap.hpp>

namespace antwika::editor
{

    [[nodiscard]] gfx::RectF tilemapPlace(
        gfx::Size canvasSize, const tilemap::Tilemap &tilemap);

    [[nodiscard]] gfx::RectF tilemapPlace(
        gfx::RectF roomRect, const tilemap::Tilemap &tilemap);

    [[nodiscard]] gfx::RectF tilemapBounds(gfx::Size canvasSize);

    [[nodiscard]] gfx::RectF tilePlace(
        const tilemap::Tilemap &tilemap,
        std::uint32_t column,
        std::uint32_t row,
        gfx::RectF whereRect);

    [[nodiscard]] std::optional<geometry::GridCell> cellAtPoint(
        const tilemap::Tilemap &tilemap,
        gfx::RectF whereRect,
        gfx::PointF point);

}
