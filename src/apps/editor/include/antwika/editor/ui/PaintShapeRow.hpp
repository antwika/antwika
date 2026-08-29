#pragma once

#include <array>
#include <vector>

#include <antwika/enums/Enumeration.hpp>
#include <antwika/geometry/GridCell.hpp>
#include <antwika/tile/TilePaint.hpp>

#include "antwika/editor/Preferences.hpp"

namespace antwika::editor
{

    using ShapePixels = std::vector<geometry::GridCell> (*)(
        geometry::GridCell fromCell, geometry::GridCell toCell);

    struct PaintShapeRow final
    {
        Paint paint;

        ShapePixels pixels;
    };

    inline constexpr std::array<PaintShapeRow, enums::kCount<Paint>>
        kPaintShapeRows{{
            {Paint::Brush, nullptr},
            {Paint::Line, tile::getLinePixels},
            {Paint::Fill, nullptr},
            {Paint::Select, nullptr},
            {Paint::Rect, tile::getRectPixels},
            {Paint::Circle, tile::getCirclePixels}}};

    static_assert(enums::tagsInOrder(kPaintShapeRows, &PaintShapeRow::paint));

    [[nodiscard]] inline ShapePixels shapePixelsOf(
        const Paint paint) noexcept
    {
        return enums::lookup(kPaintShapeRows, paint).pixels;
    }

}
