#include "antwika/tile/TilePaint.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <deque>
#include <numbers>
#include <set>
#include <utility>

#include <antwika/geometry/GridStep.hpp>
#include <antwika/tilemap/AtlasLayout.hpp>
#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/SizeF.hpp>

namespace antwika::tile
{

    namespace
    {
        [[nodiscard]] gfx::Point placeOf(
            const tilemap::Tile tile, const geometry::GridCell pixelCell)
        {
            const auto tileRect =
                tilemap::getTilePixels(
                tile.index, tilemap::tileSizeOf(tile.atlas));

            return gfx::Point{
                .x = static_cast<std::int32_t>(tileRect.originPoint.x)
                     + static_cast<std::int32_t>(pixelCell.column),
                .y = static_cast<std::int32_t>(tileRect.originPoint.y)
                     + static_cast<std::int32_t>(pixelCell.row)};
        }
    }

    std::optional<geometry::GridCell> pixelAt(
        const tilemap::Tile tile,
        const gfx::RectF whereRect,
        const gfx::PointF point)
    {
        const auto tileSize = tilemap::tileSizeOf(tile.atlas);

        if (whereRect.size.width <= 0.0F || whereRect.size.height <= 0.0F)
        {
            return std::nullopt;
        }

        const auto acrossFraction = (point.x - whereRect.originPoint.x)
                            / whereRect.size.width
                            * static_cast<float>(tileSize.width);
        const auto downFraction = (point.y - whereRect.originPoint.y)
                          / whereRect.size.height
                          * static_cast<float>(tileSize.height);

        if (acrossFraction < 0.0F || downFraction < 0.0F
            || acrossFraction >= static_cast<float>(tileSize.width)
            || downFraction >= static_cast<float>(tileSize.height))
        {
            return std::nullopt;
        }

        return geometry::GridCell{
            .column = static_cast<std::uint32_t>(acrossFraction),
            .row = static_cast<std::uint32_t>(downFraction)};
    }

    gfx::RectF getPixelPlace(
        const tilemap::Tile tile,
        const gfx::RectF whereRect,
        const geometry::GridCell pixelCell)
    {
        const auto tileSize = tilemap::tileSizeOf(tile.atlas);
        const auto width =
            whereRect.size.width / static_cast<float>(tileSize.width);
        const auto height =
            whereRect.size.height / static_cast<float>(tileSize.height);

        return gfx::RectF(
            gfx::PointF{
                whereRect.originPoint.x
                    + (static_cast<float>(pixelCell.column) * width),
                whereRect.originPoint.y
                    + (static_cast<float>(pixelCell.row) * height)},
            gfx::SizeF{width, height});
    }

    std::vector<geometry::GridCell> getLinePixels(
        const geometry::GridCell fromCell, const geometry::GridCell toCell)
    {
        const auto columnStep = static_cast<std::int64_t>(toCell.column)
                            - static_cast<std::int64_t>(fromCell.column);
        const auto rowStep = static_cast<std::int64_t>(toCell.row)
                          - static_cast<std::int64_t>(fromCell.row);
        const auto steps = std::max(
            std::abs(columnStep), std::abs(rowStep));

        std::vector<geometry::GridCell> cells;

        cells.reserve(static_cast<std::size_t>(steps) + 1);

        for (std::int64_t step = 0; step <= steps; ++step)
        {
            const auto part = steps == 0
                            ? 0.0
                            : static_cast<double>(step)
                                        / static_cast<double>(steps);

            cells.push_back(
                geometry::GridCell{
                    .column = static_cast<std::uint32_t>(
                        std::llround(
                            static_cast<double>(fromCell.column)
                            + (part * static_cast<double>(columnStep)))),
                    .row = static_cast<std::uint32_t>(
                        std::llround(
                            static_cast<double>(fromCell.row)
                            + (part * static_cast<double>(rowStep))))});
        }

        return cells;
    }

    void paintLine(
        gfx::Bitmap &atlasBitmap,
        const tilemap::Tile tile,
        const geometry::GridCell fromCell,
        const geometry::GridCell toCell,
        const gfx::Color color)
    {
        for (const auto pixel : getLinePixels(fromCell, toCell))
        {
            paint(atlasBitmap, tile, pixel, color);
        }
    }

    std::vector<geometry::GridCell> getRectPixels(
        const geometry::GridCell fromCell, const geometry::GridCell toCell)
    {
        const auto left = std::min(fromCell.column, toCell.column);
        const auto right = std::max(fromCell.column, toCell.column);
        const auto top = std::min(fromCell.row, toCell.row);
        const auto foot = std::max(fromCell.row, toCell.row);

        std::set<std::pair<std::uint32_t, std::uint32_t>> seenCells;
        std::vector<geometry::GridCell> cells;

        const auto kept = [&cells, &seenCells](
                              const std::uint32_t column,
                              const std::uint32_t row)
        {
            if (seenCells.insert({column, row}).second)
            {
                cells.push_back(
                    geometry::GridCell{
                        .column = column, .row = row});
            }
        };

        for (auto column = left; column <= right; ++column)
        {
            kept(column, top);
            kept(column, foot);
        }

        for (auto row = top; row <= foot; ++row)
        {
            kept(left, row);
            kept(right, row);
        }

        return cells;
    } // GCOVR_EXCL_LINE

    std::vector<geometry::GridCell> getCirclePixels(
        const geometry::GridCell fromCell, const geometry::GridCell toCell)
    {
        const auto left = std::min(fromCell.column, toCell.column);
        const auto right = std::max(fromCell.column, toCell.column);
        const auto top = std::min(fromCell.row, toCell.row);
        const auto foot = std::max(fromCell.row, toCell.row);
        const auto acrossArm =
            static_cast<double>(right - left) / 2.0;
        const auto downArm = static_cast<double>(foot - top) / 2.0;
        const auto middleX =
            static_cast<double>(left) + acrossArm;
        const auto middleY = static_cast<double>(top) + downArm;

        const auto steps = std::max<std::size_t>(
            8,
            static_cast<std::size_t>(
                8.0 * (acrossArm + downArm + 2.0)));

        std::set<std::pair<std::uint32_t, std::uint32_t>> seenCells;
        std::vector<geometry::GridCell> ringCells;

        for (std::size_t step = 0; step < steps; ++step)
        {
            const auto turn = 2.0 * std::numbers::pi
                              * static_cast<double>(step)
                              / static_cast<double>(steps);
            const auto column = static_cast<std::uint32_t>(
                std::llround(
                    middleX + (acrossArm * std::cos(turn))));
            const auto row = static_cast<std::uint32_t>(
                std::llround(middleY + (downArm * std::sin(turn))));

            if (seenCells.insert({column, row}).second)
            {
                ringCells.push_back(
                    geometry::GridCell{
                        .column = column, .row = row});
            }
        }

        const auto astray = [acrossArm,
                             downArm,
                             middleX,
                             middleY](const geometry::GridCell index)
        {
            const auto acrossOffset =
                acrossArm <= 0.0
                           ? 0.0
                           : (static_cast<double>(index.column) - middleX)
                          / acrossArm;
            const auto downOffset =
                downArm <= 0.0
                         ? 0.0
                         : (static_cast<double>(index.row) - middleY)
                          / downArm;

            return std::abs(
                ((acrossOffset * acrossOffset)
                 + (downOffset * downOffset))
                - 1.0);
        };

        std::vector<geometry::GridCell> cells;

        for (std::size_t index = 0; index < ringCells.size(); ++index)
        {
            const auto hereCell = ringCells[index];
            const auto thereCell = ringCells[(index + 1) % ringCells.size()];

            cells.push_back(hereCell);

            if (hereCell.column == thereCell.column
                || hereCell.row == thereCell.row)
            {
                continue;
            }

            const geometry::GridCell alongCell{
                .column = thereCell.column, .row = hereCell.row};
            const geometry::GridCell uprightCell{
                .column = hereCell.column, .row = thereCell.row};
            const auto corner = astray(alongCell) <= astray(uprightCell)
                              ? alongCell
                              : uprightCell;

            if (seenCells.insert({corner.column, corner.row}).second)
            {
                cells.push_back(corner);
            }
        }

        return cells;
    } // GCOVR_EXCL_LINE

    void paintPixels(
        gfx::Bitmap &atlasBitmap,
        const tilemap::Tile tile,
        const std::span<const geometry::GridCell> pixelCells,
        const gfx::Color color)
    {
        for (const auto pixel : pixelCells)
        {
            paint(atlasBitmap, tile, pixel, color);
        }
    }

    std::vector<geometry::GridCell> getFilledPixels(
        const gfx::Bitmap &atlasBitmap,
        const tilemap::Tile tile,
        const geometry::GridCell cell)
    {
        const auto tileSize = tilemap::tileSizeOf(tile.atlas);
        const auto was = paintedAt(atlasBitmap, tile, cell);

        std::vector<geometry::GridCell> cells;
        std::set<std::pair<std::uint32_t, std::uint32_t>> seenCells;
        std::deque<geometry::GridCell> goingCells{cell};

        seenCells.insert({cell.column, cell.row});

        while (!goingCells.empty())
        {
            const auto hereCell = goingCells.front();

            goingCells.pop_front();
            cells.push_back(hereCell);

            for (const auto step : geometry::kFourNeighbourSteps)
            {
                const auto steppedCell =
                    geometry::steppedFrom(tileSize, hereCell, step);

                if (!steppedCell.has_value())
                {
                    continue;
                }

                const auto nextCell = *steppedCell;

                if (seenCells.contains({nextCell.column, nextCell.row})
                    || paintedAt(atlasBitmap, tile, nextCell) != was)
                {
                    continue;
                }

                seenCells.insert({nextCell.column, nextCell.row});
                goingCells.push_back(nextCell);
            }
        }

        return cells;
    }

    void paintFill(
        gfx::Bitmap &atlasBitmap,
        const tilemap::Tile tile,
        const geometry::GridCell cell,
        const gfx::Color color)
    {
        for (const auto pixel : getFilledPixels(atlasBitmap, tile, cell))
        {
            paint(atlasBitmap, tile, pixel, color);
        }
    }

    void paint(
        gfx::Bitmap &atlasBitmap,
        const tilemap::Tile tile,
        const geometry::GridCell pixelCell,
        const gfx::Color color)
    {
        const auto pixelRect = placeOf(tile, pixelCell);

        gfx::setColorAt(atlasBitmap, pixelRect.x, pixelRect.y, color);
    }

    gfx::Color paintedAt(
        const gfx::Bitmap &atlasBitmap,
        const tilemap::Tile tile,
        const geometry::GridCell pixelCell)
    {
        const auto pixelRect = placeOf(tile, pixelCell);

        return gfx::colorAt(atlasBitmap, pixelRect.x, pixelRect.y)
            .value_or(gfx::Color{.alpha = 0});
    } // GCOVR_EXCL_LINE

    bool isSoleInk(
        const std::span<const gfx::Color> paletteColors,
        const std::size_t which)
    {
        for (std::size_t other = 0; other < paletteColors.size(); ++other)
        {
            if (other != which
                && paletteColors[other].red == paletteColors[which].red
                && paletteColors[other].green == paletteColors[which].green
                && paletteColors[other].blue == paletteColors[which].blue)
            {
                return false;
            }
        }

        return true;
    }

    gfx::Color soleInkColorOf(
        const std::span<const gfx::Color> paletteColors,
        const std::size_t which,
        const gfx::Color wantedColor)
    {
        const auto taken = [&paletteColors, which](const gfx::Color color)
        {
            for (std::size_t other = 0; other < paletteColors.size(); ++other)
            {
                if (other != which
                    && paletteColors[other].red == color.red
                    && paletteColors[other].green == color.green
                    && paletteColors[other].blue == color.blue)
                {
                    return true;
                }
            }

            return false;
        };

        if (!taken(wantedColor))
        {
            return wantedColor;
        }

        for (std::int32_t distance = 1; distance < 256; ++distance)
        {
            for (const auto blue :
                 {static_cast<std::int32_t>(wantedColor.blue) + distance,
                  static_cast<std::int32_t>(wantedColor.blue) - distance})
            {
                if (blue < 0 || blue > 255)
                {
                    continue;
                }

                auto shiftedColor = wantedColor;

                shiftedColor.blue = static_cast<std::uint8_t>(blue);

                if (!taken(shiftedColor))
                {
                    return shiftedColor;
                }
            }
        }

        return wantedColor; // GCOVR_EXCL_LINE
    }

    std::vector<std::size_t> getPaintedWith(
        const gfx::Bitmap &atlasBitmap, const gfx::Color color)
    {
        std::vector<std::size_t> places{};

        for (std::size_t index = 0;
             index + 3 < atlasBitmap.pixels.size();
             index += gfx::kBytesPerPixel)
        {
            if (atlasBitmap.pixels[index + 3] != 0
                && atlasBitmap.pixels[index] == color.red
                && atlasBitmap.pixels[index + 1] == color.green
                && atlasBitmap.pixels[index + 2] == color.blue)
            {
                places.push_back(index);
            }
        }

        return places;
    } // GCOVR_EXCL_LINE

    void repaintAt(
        gfx::Bitmap &atlasBitmap,
        const std::span<const std::size_t> places,
        const gfx::Color color)
    {
        for (const auto index : places)
        {
            atlasBitmap.pixels[index] = color.red;
            atlasBitmap.pixels[index + 1] = color.green;
            atlasBitmap.pixels[index + 2] = color.blue;
        }
    }

}
