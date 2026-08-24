#include "antwika/editor/editor/state/SheetView.hpp"

#include <antwika/camera/FlyCamera.hpp>

#include "antwika/editor/ui/AtlasView.hpp"
#include "antwika/editor/ui/TilemapView.hpp"

namespace antwika::editor
{

    gfx::RectF SheetView::getClipRect() const
    {
        return sheetRect.value_or(getTilemapBounds(camera::kCanvasSize));
    }

    gfx::RectF SheetView::getFrameRect() const
    {
        return canvasRect.value_or(
            getInspectColumnBounds(camera::kCanvasSize));
    }

    gfx::RectF SheetView::getGridRect(
        const tilemap::Tilemap &drawnTilemap) const
    {
        return getPanZoomed(
            getTilemapPlace(getClipRect(), drawnTilemap), panPoint, zoom);
    }


    std::optional<geometry::GridCell> SheetView::getCellUnder(
        const tilemap::Tilemap &drawnTilemap,
        const gfx::PointF pointerOnCanvas) const
    {
        return cellShownAt(
            drawnTilemap,
            getGridRect(drawnTilemap),
            getClipRect(),
            pointerOnCanvas);
    }

}
