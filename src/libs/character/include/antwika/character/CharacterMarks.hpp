#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include <antwika/geometry/Grid.hpp>
#include <antwika/gfx/Bitmap.hpp>
#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/RectF.hpp>
#include <antwika/gfx/Size.hpp>

#include "antwika/character/PixelBuffer.hpp"
#include "antwika/character/PixelSelection.hpp"

namespace antwika::character
{

    [[nodiscard]] geometry::GridCell getSelectionOrigin(
        PixelSelection selection);

    [[nodiscard]] gfx::Size getSelectionSize(PixelSelection selection);

    [[nodiscard]] bool isSelectionContains(
        PixelSelection selection, geometry::GridCell pixelCell);

    [[nodiscard]] PixelSelection getMovedSelection(
        PixelSelection selection, std::int32_t column, std::int32_t row);

    [[nodiscard]] PixelBuffer copiedFrom(
        const gfx::Bitmap &sheetBitmap,
        std::size_t direction,
        std::size_t frame,
        PixelSelection selection);

    [[nodiscard]] PixelBuffer cutFrom(
        gfx::Bitmap &sheetBitmap,
        std::size_t direction,
        std::size_t frame,
        PixelSelection selection);

    [[nodiscard]] PixelBuffer getMirroredHorizontally(
        const PixelBuffer &buffer);

    void pasteInto(
        gfx::Bitmap &sheetBitmap,
        std::size_t direction,
        std::size_t frame,
        geometry::GridCell cell,
        const PixelBuffer &buffer);

    [[nodiscard]] gfx::RectF getSelectionRect(
        gfx::RectF whereRect, PixelSelection selection);

}
