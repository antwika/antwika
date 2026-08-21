#include "antwika/editor/ui/TilemapView.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>

#include <antwika/gfx/SizeF.hpp>

#include "antwika/editor/ui/EditorLook.hpp"

namespace antwika::editor
{

    gfx::RectF tilemapBounds(const gfx::Size canvasSize)
    {
        return gfx::RectF(
            gfx::PointF{kToolPanelWidth, kTopBarHeight},
            gfx::SizeF{
                static_cast<float>(canvasSize.width) - kToolPanelWidth
                    - kRightPanelWidth - kInspectColumnWidth,
                static_cast<float>(canvasSize.height) - kTopBarHeight
                    - kBottomBarHeight});
    }

    gfx::RectF tilemapPlace(
        const gfx::Size canvasSize, const tilemap::Tilemap &tilemap)
    {
        return tilemapPlace(tilemapBounds(canvasSize), tilemap);
    }

    gfx::RectF tilemapPlace(
        const gfx::RectF roomRect, const tilemap::Tilemap &tilemap)
    {
        const auto cell = tilemap::gridCellSize();
        const auto tilemapWidth = static_cast<float>(
            tilemap.columns * cell.width);
        const auto tilemapHeight =
            static_cast<float>(tilemap.rows * cell.height);

        const auto roomWidth =
            roomRect.size.width - (2.0F * kPaneMargin);
        const auto roomHeight =
            roomRect.size.height - (2.0F * kPaneMargin);

        if (tilemapWidth <= 0.0F || tilemapHeight <= 0.0F)
        {
            return gfx::RectF(
                gfx::PointF{
                    roomRect.originPoint.x + kPaneMargin,
                    roomRect.originPoint.y},
                gfx::SizeF{0.0F, 0.0F});
        }

        const auto scale = std::min(roomWidth / tilemapWidth,
            roomHeight / tilemapHeight);
        const auto width = tilemapWidth * scale;
        const auto height = tilemapHeight * scale;

        return gfx::RectF(
            gfx::PointF{
                roomRect.originPoint.x + kPaneMargin
                    + ((roomWidth - width) / 2.0F),
                roomRect.originPoint.y + kPaneMargin
                    + ((roomHeight - height) / 2.0F)},
            gfx::SizeF{width, height});
    }

    std::optional<geometry::GridCell> cellAtPoint(
        const tilemap::Tilemap &tilemap,
        const gfx::RectF whereRect,
        const gfx::PointF point)
    {
        if (tilemap.columns == 0 || tilemap.rows == 0
            || whereRect.size.width <= 0.0F
            || whereRect.size.height <= 0.0F)
        {
            return std::nullopt;
        }

        const auto cellWidth =
            whereRect.size.width / static_cast<float>(tilemap.columns);
        const auto cellHeight =
            whereRect.size.height / static_cast<float>(tilemap.rows);

        const auto column = (point.x - whereRect.originPoint.x) / cellWidth;
        const auto row = (point.y - whereRect.originPoint.y) / cellHeight;

        if (column < 0.0F || row < 0.0F
            || column >= static_cast<float>(tilemap.columns)
            || row >= static_cast<float>(tilemap.rows))
        {
            return std::nullopt;
        }

        return geometry::GridCell{
            .column = static_cast<std::uint32_t>(column),
            .row = static_cast<std::uint32_t>(row)};
    }

    gfx::RectF tilePlace(
        const tilemap::Tilemap &tilemap,
        const std::uint32_t column,
        const std::uint32_t row,
        const gfx::RectF whereRect)
    {
        const auto cellWidth =
            whereRect.size.width / static_cast<float>(tilemap.columns);
        const auto cellHeight =
            whereRect.size.height / static_cast<float>(tilemap.rows);

        const auto cell = tilemap.at(column, row);
        const auto tile = tilemap::tileSizeOf(
            cell.has_value() ? cell->atlas : tilemap::Atlas::Floor);
        const auto scale = std::min(
            cellWidth / static_cast<float>(tile.width),
            cellHeight / static_cast<float>(tile.height));
        const auto width = static_cast<float>(tile.width) * scale;
        const auto height = static_cast<float>(tile.height) * scale;

        const auto left =
            whereRect.originPoint.x + (static_cast<float>(column) * cellWidth);
        const auto top =
            whereRect.originPoint.y + (static_cast<float>(row) * cellHeight);

        return gfx::RectF(
            gfx::PointF{
                left + ((cellWidth - width) / 2.0F),
                top + ((cellHeight - height) / 2.0F)},
            gfx::SizeF{width, height});
    }
}
