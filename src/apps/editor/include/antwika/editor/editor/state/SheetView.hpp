#pragma once

#include <optional>

#include <antwika/geometry/Grid.hpp>
#include <antwika/gfx/PointF.hpp>
#include <antwika/gfx/RectF.hpp>
#include <antwika/tilemap/Tilemap.hpp>

namespace antwika::editor
{

    struct SheetView final
    {
        [[nodiscard]] gfx::RectF getClipRect() const;

        [[nodiscard]] gfx::RectF getFrameRect() const;

        [[nodiscard]] gfx::RectF getGridRect(
            const tilemap::Tilemap &drawnTilemap) const;

        [[nodiscard]] std::optional<geometry::GridCell> getCellUnder(
            const tilemap::Tilemap &drawnTilemap,
            gfx::PointF pointerOnCanvas) const;

        float zoom = 1.0F;

        gfx::PointF panPoint{};

        std::optional<gfx::RectF> sheetRect;

        std::optional<gfx::RectF> canvasRect;
    };

}
