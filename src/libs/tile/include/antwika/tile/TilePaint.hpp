#pragma once

#include <array>
#include <cstddef>
#include <optional>
#include <span>
#include <vector>

#include <antwika/geometry/Grid.hpp>
#include <antwika/gfx/Bitmap.hpp>
#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/PointF.hpp>
#include <antwika/gfx/RectF.hpp>
#include <antwika/ui/WidgetId.hpp>
#include <antwika/gfx/Size.hpp>

#include <antwika/tilemap/Tilemap.hpp>

namespace antwika::tile
{

    inline constexpr std::size_t kPaletteSize = 6;

    inline constexpr std::size_t kMaxInks = 16;

    inline constexpr std::array<gfx::Color, kPaletteSize> kPaletteColors{
        gfx::Color{.red = 0, .green = 39, .blue = 43},
        gfx::Color{.red = 49, .green = 56, .blue = 28},
        gfx::Color{.red = 3, .green = 82, .blue = 63},
        gfx::Color{.red = 133, .green = 98, .blue = 65},
        gfx::Color{.red = 159, .green = 166, .blue = 108},
        gfx::Color{.red = 217, .green = 211, .blue = 152}};

    inline constexpr float kSwatchSide = 15.0F;

    inline constexpr float kSwatchGap = 3.0F;

    [[nodiscard]] ui::WidgetId swatchWidget(std::size_t which);

    [[nodiscard]] gfx::RectF pixelPlace(
        tilemap::Tile tile, gfx::RectF whereRect, geometry::GridCell pixelCell);

    [[nodiscard]] std::optional<geometry::GridCell> pixelAt(
        tilemap::Tile tile, gfx::RectF whereRect, gfx::PointF point);

    void paint(
        gfx::Bitmap &atlasBitmap,
        tilemap::Tile tile,
        geometry::GridCell pixelCell,
        gfx::Color color);

    [[nodiscard]] std::vector<geometry::GridCell> linePixels(
        geometry::GridCell fromCell, geometry::GridCell toCell);

    [[nodiscard]] std::vector<geometry::GridCell> rectPixels(
        geometry::GridCell fromCell, geometry::GridCell toCell);

    [[nodiscard]] std::vector<geometry::GridCell> circlePixels(
        geometry::GridCell fromCell, geometry::GridCell toCell);

    void paintPixels(
        gfx::Bitmap &atlasBitmap,
        tilemap::Tile tile,
        std::span<const geometry::GridCell> pixelCells,
        gfx::Color color);

    void paintLine(
        gfx::Bitmap &atlasBitmap,
        tilemap::Tile tile,
        geometry::GridCell fromCell,
        geometry::GridCell toCell,
        gfx::Color color);

    [[nodiscard]] std::vector<geometry::GridCell> filledPixels(
        const gfx::Bitmap &atlasBitmap,
        tilemap::Tile tile,
        geometry::GridCell cell);

    void paintFill(
        gfx::Bitmap &atlasBitmap,
        tilemap::Tile tile,
        geometry::GridCell cell,
        gfx::Color color);

    [[nodiscard]] gfx::Color paintedAt(
        const gfx::Bitmap &atlasBitmap,
        tilemap::Tile tile,
        geometry::GridCell pixelCell);

    [[nodiscard]] bool soleInk(
        std::span<const gfx::Color> paletteColors, std::size_t which);

    [[nodiscard]] std::vector<std::size_t> paintedWith(
        const gfx::Bitmap &atlasBitmap, gfx::Color color);

    void repaintAt(
        gfx::Bitmap &atlasBitmap,
        std::span<const std::size_t> places,
        gfx::Color color);

}
