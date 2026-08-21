#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>
#include <string>

#include <antwika/geometry/Grid.hpp>
#include <antwika/gfx/Bitmap.hpp>
#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/PointF.hpp>
#include <antwika/gfx/Rect.hpp>
#include <antwika/gfx/RectF.hpp>
#include <antwika/gfx/Size.hpp>

namespace antwika::editor
{

    inline constexpr gfx::Size kIconCellSize{16, 16};

    inline constexpr std::string_view kIconSheet = "icons-16.png";

    inline constexpr std::size_t kIconColumns = 6;

    inline constexpr float kIconGridScale = 2.0F;

    inline constexpr float kEditedIconScale = 8.0F;

    inline constexpr float kIconSheetLeft = 64.0F;

    [[nodiscard]] std::size_t iconCount(gfx::Size sheetSize);

    [[nodiscard]] gfx::Rect iconSource(std::size_t iconIndex);

    [[nodiscard]] gfx::RectF iconCellRect(
        gfx::Size canvasSize, std::size_t count, std::size_t iconIndex);

    [[nodiscard]] std::optional<std::size_t> iconCellAt(
        gfx::Size canvasSize,
        std::size_t count,
        gfx::PointF point);

    [[nodiscard]] gfx::RectF editedIconRect(gfx::Size canvasSize);

    [[nodiscard]] gfx::RectF iconPixelRect(
        gfx::RectF roomRect, geometry::GridCell pixelCell);

    [[nodiscard]] std::optional<geometry::GridCell> iconPixelAt(
        gfx::RectF roomRect, gfx::PointF point);

    [[nodiscard]] gfx::Color iconPixelColor(
        const gfx::Bitmap &sheetBitmap,
        std::size_t iconIndex,
        geometry::GridCell pixelCell);

    void setIconPixel(
        gfx::Bitmap &sheetBitmap,
        std::size_t iconIndex,
        geometry::GridCell pixelCell,
        gfx::Color inkColor);

    [[nodiscard]] gfx::Bitmap loadIconSheet(
        const std::string &mapPath, std::string_view app);

}
