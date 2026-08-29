#include "antwika/editor/ui/IconSheet.hpp"

#include <algorithm>

#include <antwika/gfx/GfxError.hpp>
#include <antwika/gfx/SizeF.hpp>

#include <antwika/assets/MapAssets.hpp>

#include "antwika/editor/ui/EditorLook.hpp"

namespace antwika::editor
{

    namespace
    {
        constexpr float kCellGap = 2.0F;

        [[nodiscard]] float getCellSide()
        {
            return static_cast<float>(kIconCellSize.width)
                   * kIconGridScale;
        }

        [[nodiscard]] std::size_t getRowCount(const std::size_t count)
        {
            return (count + kIconColumns - 1) / kIconColumns;
        }

        [[nodiscard]] float getGridWide()
        {
            return (static_cast<float>(kIconColumns)
                    * (getCellSide() + kCellGap))
                   - kCellGap;
        }

        [[nodiscard]] float getGridTall(const std::size_t count)
        {
            return (static_cast<float>(getRowCount(count))
                    * (getCellSide() + kCellGap))
                   - kCellGap;
        }

        [[nodiscard]] float getSheetScale(
            const gfx::RectF sheetRect, const std::size_t count)
        {
            if (count == 0)
            {
                return 1.0F;
            }

            return std::min(
                sheetRect.size.width / getGridWide(),
                sheetRect.size.height / getGridTall(count));
        }
    }

    gfx::RectF getIconSheetBounds(const gfx::Size canvasSize)
    {
        return gfx::RectF(
            gfx::PointF{kIconSheetLeft, 0.0F},
            gfx::SizeF{
                getGridWide(), static_cast<float>(canvasSize.height)});
    }

    gfx::RectF getIconDrawBounds(const gfx::Size canvasSize)
    {
        const auto side =
            static_cast<float>(kIconCellSize.width) * kEditedIconScale;
        const auto sheetWide =
            static_cast<float>(kIconColumns) * (getCellSide() + kCellGap);

        return gfx::RectF(
            gfx::PointF{
                kIconSheetLeft + sheetWide + getCellSide(), 0.0F},
            gfx::SizeF{side, static_cast<float>(canvasSize.height)});
    }

    std::size_t getIconCount(const gfx::Size sheetSize)
    {
        if (kIconCellSize.width == 0)
        {
            return 0;
        }

        return sheetSize.width / kIconCellSize.width;
    }

    gfx::Rect getIconSource(const std::size_t iconIndex)
    {
        return gfx::Rect{
            .originPoint =
                {.x = static_cast<std::int32_t>(
                     iconIndex * kIconCellSize.width),
                 .y = 0},
            .size = kIconCellSize};
    }

    gfx::RectF getIconCellRect(
        const gfx::RectF sheetRect,
        const std::size_t count,
        const std::size_t iconIndex)
    {
        const auto column = iconIndex % kIconColumns;
        const auto row = iconIndex / kIconColumns;
        const auto scale = getSheetScale(sheetRect, count);
        const auto side = getCellSide() * scale;
        const auto gap = kCellGap * scale;
        const auto gridWide =
            (static_cast<float>(kIconColumns) * (side + gap)) - gap;
        const auto gridTall =
            (static_cast<float>(getRowCount(count)) * (side + gap)) - gap;

        return gfx::RectF(
            gfx::PointF{
                sheetRect.originPoint.x
                    + ((sheetRect.size.width - gridWide) / 2.0F)
                    + (static_cast<float>(column) * (side + gap)),
                sheetRect.originPoint.y
                    + ((sheetRect.size.height - gridTall) / 2.0F)
                    + (static_cast<float>(row) * (side + gap))},
            gfx::SizeF{side, side});
    }

    std::optional<std::size_t> iconCellAt(
        const gfx::RectF sheetRect,
        const std::size_t count,
        const gfx::PointF point)
    {
        for (std::size_t index = 0; index < count; ++index)
        {
            const auto where =
                getIconCellRect(sheetRect, count, index);

            if (point.x >= where.originPoint.x
                && point.y >= where.originPoint.y
                && point.x < where.originPoint.x + where.size.width
                && point.y
                       < where.originPoint.y + where.size.height)
            {
                return index;
            }
        }

        return std::nullopt;
    }

    gfx::RectF getEditedIconRect(const gfx::RectF drawRect)
    {
        const auto roomTall = std::max(
            drawRect.size.height - kPaneMargin, 0.0F);
        const auto side = std::min(drawRect.size.width, roomTall);

        return gfx::RectF(
            gfx::PointF{
                drawRect.originPoint.x + drawRect.size.width - side,
                drawRect.originPoint.y + kPaneMargin},
            gfx::SizeF{side, side});
    }

    gfx::RectF getIconPixelRect(
        const gfx::RectF roomRect, const geometry::GridCell pixelCell)
    {
        const auto width =
            roomRect.size.width
            / static_cast<float>(kIconCellSize.width);
        const auto height =
            roomRect.size.height
            / static_cast<float>(kIconCellSize.height);

        return gfx::RectF(
            gfx::PointF{
                roomRect.originPoint.x
                    + (static_cast<float>(pixelCell.column)
                       * width),
                roomRect.originPoint.y
                    + (static_cast<float>(pixelCell.row) * height)},
            gfx::SizeF{width, height});
    }

    std::optional<geometry::GridCell> iconPixelAt(
        const gfx::RectF roomRect, const gfx::PointF point)
    {
        if (roomRect.size.width <= 0.0F
            || roomRect.size.height <= 0.0F)
        {
            return std::nullopt;
        }

        const auto acrossFraction =
            (point.x - roomRect.originPoint.x) / roomRect.size.width
            * static_cast<float>(kIconCellSize.width);
        const auto downFraction =
            (point.y - roomRect.originPoint.y) / roomRect.size.height
            * static_cast<float>(kIconCellSize.height);

        if (acrossFraction < 0.0F || downFraction < 0.0F
            || acrossFraction >= static_cast<float>(kIconCellSize.width)
            || downFraction >= static_cast<float>(kIconCellSize.height))
        {
            return std::nullopt;
        }

        return geometry::GridCell{
            static_cast<std::uint32_t>(acrossFraction),
            static_cast<std::uint32_t>(downFraction)};
    }

    gfx::Color getIconPixelColor(
        const gfx::Bitmap &sheetBitmap,
        const std::size_t iconIndex,
        const geometry::GridCell pixelCell)
    {
        return gfx::colorAt(
                   sheetBitmap,
                   static_cast<std::int32_t>(
                       (iconIndex * kIconCellSize.width) + pixelCell.column),
                   static_cast<std::int32_t>(pixelCell.row))
            .value_or(gfx::Color{.alpha = 0});
    }

    void setIconPixel(
        gfx::Bitmap &sheetBitmap,
        const std::size_t iconIndex,
        const geometry::GridCell pixelCell,
        const gfx::Color inkColor)
    {
        gfx::setColorAt(
            sheetBitmap,
            static_cast<std::int32_t>(
                (iconIndex * kIconCellSize.width) + pixelCell.column),
            static_cast<std::int32_t>(pixelCell.row),
            inkColor);
    }

    gfx::Bitmap getLoadIconSheet(
        const std::string &mapPath, const std::string_view app)
    {
        auto sheet =
            assets::getReadSharedOrBundled(mapPath, kIconSheet, app);

        if (sheet.size.height != kIconCellSize.height
            || sheet.size.width % kIconCellSize.width != 0
            || sheet.size.width == 0)
        {
            throw gfx::GfxError(
                "the icon sheet is not a row of icon cells");
        }

        return sheet;
    } // GCOVR_EXCL_LINE

}
