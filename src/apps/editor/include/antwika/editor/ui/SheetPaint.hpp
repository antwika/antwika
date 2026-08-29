#pragma once

#include <array>
#include <functional>
#include <span>

#include <antwika/geometry/GridCell.hpp>
#include <antwika/tile/TilePaint.hpp>

#include "antwika/editor/Preferences.hpp"
#include "antwika/editor/editor/state/SheetStroke.hpp"
#include "antwika/editor/ui/PaintShapeRow.hpp"
#include "antwika/editor/view/IEditSteps.hpp"

namespace antwika::editor
{

    struct PaintSurface final
    {
        std::function<void(std::span<const geometry::GridCell>)> paintCells;

        std::function<void(geometry::GridCell)> paintFill;

        std::function<void()> touch;
    };

    inline void beginStroke(
        const Paint paint,
        const geometry::GridCell pixelCell,
        SheetStroke &stroke,
        const PaintSurface &surface)
    {
        if (paint == Paint::Fill)
        {
            surface.paintFill(pixelCell);
        }
        else
        {
            surface.paintCells(std::array{pixelCell});
            stroke.brushAtCell = pixelCell;
            stroke.active = true;
        }

        surface.touch();
    }

    inline void dragStroke(
        const geometry::GridCell pixelCell,
        SheetStroke &stroke,
        const PaintSurface &surface)
    {
        surface.paintCells(
            tile::getLinePixels(
                stroke.brushAtCell.value_or(pixelCell), pixelCell));
        stroke.brushAtCell = pixelCell;
        surface.touch();
    }

    inline void endShapedStroke(
        const Paint paint,
        const geometry::GridCell pixelCell,
        const SheetStroke &stroke,
        const PaintSurface &surface,
        IEditSteps &editSteps)
    {
        if (!stroke.lineFromCell.has_value())
        {
            return;
        }

        editSteps.pushUndo();

        const auto pixels = shapePixelsOf(paint);

        surface.paintCells(pixels(*stroke.lineFromCell, pixelCell));
        surface.touch();
    }

}
