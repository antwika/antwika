#include "antwika/character/CharacterMarks.hpp"

#include <algorithm>
#include <cstddef>

#include <antwika/gfx/SizeF.hpp>

#include "antwika/character/Character.hpp"

namespace antwika::character
{


    geometry::GridCell getSelectionOrigin(const PixelSelection selection)
    {
        return geometry::GridCell{
            std::min(selection.fromCell.column, selection.toCell.column),
            std::min(selection.fromCell.row, selection.toCell.row)};
    }

    gfx::Size getSelectionSize(const PixelSelection selection)
    {
        return gfx::Size{
            .width =
                std::max(selection.fromCell.column, selection.toCell.column)
                - std::min(
                    selection.fromCell.column,
                    selection.toCell.column) + 1,
            .height =
                std::max(selection.fromCell.row, selection.toCell.row)
                - std::min(selection.fromCell.row, selection.toCell.row) + 1};
    }

    bool isSelectionContains(
        const PixelSelection selection,
        const geometry::GridCell pixelCell)
    {
        const auto corner = getSelectionOrigin(selection);
        const auto size = getSelectionSize(selection);

        return pixelCell.column >= corner.column
               && pixelCell.row >= corner.row
               && pixelCell.column < corner.column + size.width
               && pixelCell.row < corner.row + size.height;
    }

    PixelSelection getMovedSelection(
        const PixelSelection selection,
        const std::int32_t column,
        const std::int32_t row)
    {
        const auto corner = getSelectionOrigin(selection);
        const auto size = getSelectionSize(selection);
        const auto clamped = [](const std::uint32_t was,
                                const std::int32_t step,
                                const std::uint32_t span,
                                const std::uint32_t extent)
        {
            const auto askedValue = static_cast<std::int64_t>(was) + step;
            const auto most = static_cast<std::int64_t>(extent)
                              - static_cast<std::int64_t>(span);

            return static_cast<std::uint32_t>(
                std::clamp<std::int64_t>(
                    askedValue, 0, std::max<std::int64_t>(most, 0)));
        };
        const auto left = clamped(
            corner.column, column, size.width, kCharacterCellSize.width);
        const auto top = clamped(
            corner.row, row, size.height, kCharacterCellSize.height);

        return PixelSelection{
            .fromCell = geometry::GridCell{left, top},
            .toCell = geometry::GridCell{
                left + size.width - 1, top + size.height - 1}};
    }

    PixelBuffer copiedFrom(
        const gfx::Bitmap &sheetBitmap,
        const std::size_t direction,
        const std::size_t frame,
        const PixelSelection selection)
    {
        const auto left =
            std::min(selection.fromCell.column, selection.toCell.column);
        const auto right =
            std::max(
                selection.fromCell.column,
                selection.toCell.column);
        const auto top = std::min(selection.fromCell.row, selection.toCell.row);
        const auto foot = std::max(selection.fromCell.row,
            selection.toCell.row);

        PixelBuffer pixels{
            .size =
                gfx::Size{
                    .width = right - left + 1,
                    .height = foot - top + 1},
            .pixelColors = {}};

        for (auto row = top; row <= foot; ++row)
        {
            for (auto column = left; column <= right; ++column)
            {
                pixels.pixelColors.push_back(
                    getCharacterPixelColor(
                        sheetBitmap,
                        direction,
                        frame,
                        geometry::GridCell{column, row}));
            }
        }

        return pixels;
    } // GCOVR_EXCL_LINE

    PixelBuffer cutFrom(
        gfx::Bitmap &sheetBitmap,
        const std::size_t direction,
        const std::size_t frame,
        const PixelSelection selection)
    {
        auto copiedBitmap = copiedFrom(sheetBitmap, direction, frame,
        selection);
        const auto corner = getSelectionOrigin(selection);
        const auto size = getSelectionSize(selection);

        for (std::uint32_t row = 0; row < size.height; ++row)
        {
            for (std::uint32_t column = 0; column < size.width;
                 ++column)
            {
                paintCharacter(
                    sheetBitmap,
                    direction,
                    frame,
                    geometry::GridCell{
                        corner.column + column, corner.row + row},
                    gfx::Color{
                        .red = 0,
                        .green = 0,
                        .blue = 0,
                        .alpha = 0});
            }
        }

        return copiedBitmap;
    } // GCOVR_EXCL_LINE

    PixelBuffer getMirroredHorizontally(const PixelBuffer &buffer)
    {
        PixelBuffer pixels{.size = buffer.size, .pixelColors = {}};

        pixels.pixelColors.reserve(buffer.pixelColors.size());

        for (std::uint32_t row = 0; row < buffer.size.height; ++row)
        {
            for (std::uint32_t column = 0; column < buffer.size.width;
                 ++column)
            {
                pixels.pixelColors.push_back(
                    buffer.pixelColors.at(
                        (static_cast<std::size_t>(row)
                         * buffer.size.width)
                        + (buffer.size.width - column - 1)));
            }
        }

        return pixels;
    } // GCOVR_EXCL_LINE

    void pasteInto(
        gfx::Bitmap &sheetBitmap,
        const std::size_t direction,
        const std::size_t frame,
        const geometry::GridCell cell,
        const PixelBuffer &buffer)
    {
        for (std::uint32_t row = 0; row < buffer.size.height; ++row)
        {
            for (std::uint32_t column = 0; column < buffer.size.width;
                 ++column)
            {
                const geometry::GridCell onCell{
                    cell.column + column, cell.row + row};

                if (onCell.column >= kCharacterCellSize.width
                    || onCell.row >= kCharacterCellSize.height)
                {
                    continue;
                }

                paintCharacter(
                    sheetBitmap,
                    direction,
                    frame,
                    onCell,
                    buffer.pixelColors.at(
                        (static_cast<std::size_t>(row)
                         * buffer.size.width)
                        + column));
            }
        }
    }

    gfx::RectF getSelectionRect(
        const gfx::RectF whereRect, const PixelSelection selection)
    {
        const auto corner = getSelectionOrigin(selection);
        const auto size = getSelectionSize(selection);
        const auto first = getCharacterPixelPlace(whereRect, corner);
        const auto lastRect = getCharacterPixelPlace(
            whereRect,
            geometry::GridCell{
                corner.column + size.width - 1,
                corner.row + size.height - 1});

        return gfx::RectF(
            first.originPoint,
            gfx::SizeF{
                lastRect.originPoint.x + lastRect.size.width
                    - first.originPoint.x,
                lastRect.originPoint.y + lastRect.size.height
                    - first.originPoint.y});
    }

}
