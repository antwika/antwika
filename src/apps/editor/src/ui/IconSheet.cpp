#include "antwika/editor/ui/IconSheet.hpp"

#include <antwika/gfx/GfxError.hpp>
#include <antwika/gfx/SizeF.hpp>

#include <antwika/map/MapAssets.hpp>

namespace antwika::editor
{

    namespace
    {
        constexpr float kCellGap = 2.0F;

        [[nodiscard]] float cellSide()
        {
            return static_cast<float>(kIconCellSize.width)
                   * kIconGridScale;
        }

        [[nodiscard]] float sheetTop(
            const gfx::Size canvasSize, const std::size_t count)
        {
            const auto rows =
                (count + kIconColumns - 1) / kIconColumns;
            const auto height =
                static_cast<float>(rows)
                * (cellSide() + kCellGap);

            return (static_cast<float>(canvasSize.height) - height)
                   / 2.0F;
        }
    }

    std::size_t iconCount(const gfx::Size sheetSize)
    {
        if (kIconCellSize.width == 0)
        {
            return 0;
        }

        return sheetSize.width / kIconCellSize.width;
    }

    gfx::Rect iconSource(const std::size_t iconIndex)
    {
        return gfx::Rect{
            .originPoint =
                {.x = static_cast<std::int32_t>(
                     iconIndex * kIconCellSize.width),
                 .y = 0},
            .size = kIconCellSize};
    }

    gfx::RectF iconCellRect(
        const gfx::Size canvasSize,
        const std::size_t count,
        const std::size_t iconIndex)
    {
        const auto column = iconIndex % kIconColumns;
        const auto row = iconIndex / kIconColumns;

        return gfx::RectF(
            gfx::PointF{
                kIconSheetLeft
                    + (static_cast<float>(column)
                       * (cellSide() + kCellGap)),
                sheetTop(canvasSize, count)
                    + (static_cast<float>(row)
                       * (cellSide() + kCellGap))},
            gfx::SizeF{cellSide(), cellSide()});
    }

    std::optional<std::size_t> iconCellAt(
        const gfx::Size canvasSize,
        const std::size_t count,
        const gfx::PointF point)
    {
        for (std::size_t index = 0; index < count; ++index)
        {
            const auto where =
                iconCellRect(canvasSize, count, index);

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

    gfx::RectF editedIconRect(const gfx::Size canvasSize)
    {
        const auto side =
            static_cast<float>(kIconCellSize.width)
            * kEditedIconScale;
        const auto sheetWide =
            static_cast<float>(kIconColumns)
            * (cellSide() + kCellGap);

        return gfx::RectF(
            gfx::PointF{
                kIconSheetLeft + sheetWide + cellSide(),
                (static_cast<float>(canvasSize.height) - side)
                    / 2.0F},
            gfx::SizeF{side, side});
    }

    gfx::RectF iconPixelRect(
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

    gfx::Color iconPixelColor(
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

    gfx::Bitmap loadIconSheet(
        const std::string &mapPath, const std::string_view app)
    {
        auto sheet =
            map::readSharedOrBundled(mapPath, kIconSheet, app);

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
