#include "antwika/tilemap/Tilemap.hpp"

#include <algorithm>
#include <cstddef>
#include <utility>

#include <antwika/gfx/PointF.hpp>
#include <antwika/gfx/SizeF.hpp>

#include "antwika/tilemap/AtlasLayout.hpp"

namespace antwika::tilemap
{

    bool Tilemap::isComplete() const
    {
        return tiles.size()
               == static_cast<std::size_t>(columns) * rows;
    }

    std::optional<Tile> Tilemap::at(
        const std::uint32_t column, const std::uint32_t row) const
    {
        if (column >= columns || row >= rows)
        {
            return std::nullopt;
        }

        const auto place =
            (static_cast<std::size_t>(row) * columns) + column;

        if (place >= tiles.size())
        {
            return std::nullopt;
        }

        return tiles[place];
    }

    gfx::Size tileSizeOf(const Atlas atlas)
    {
        return atlas == Atlas::Floor ? kFloorTileSize : kWallTileSize;
    }

    gfx::RectF tileSource(const Tile tile)
    {
        return tilePixels(tile.index, tileSizeOf(tile.atlas));
    }

    Tilemap defaultTilemap()
    {
        const auto columnCount = static_cast<std::uint32_t>(kAtlasColumns);
        const auto rowCount = static_cast<std::uint32_t>(kAtlasRows);

        Tilemap tilemap;

        tilemap.columns = columnCount * 2;
        tilemap.rows = rowCount;
        tilemap.tiles.reserve(
            static_cast<std::size_t>(tilemap.columns) * tilemap.rows);

        for (std::uint32_t row = 0; row < tilemap.rows; ++row)
        {
            for (std::uint32_t column = 0; column < tilemap.columns;
                 ++column)
            {
                const auto side =
                    column < columnCount ? Atlas::Wall : Atlas::Floor;
                const auto wrappedColumn =
                    column < columnCount ? column : column - columnCount;

                tilemap.tiles.push_back(
                    Tile{
                        .atlas = side,
                        .index = static_cast<std::uint16_t>(
                            (row * columnCount) + wrappedColumn)});
            }
        }

        return tilemap;
    } // GCOVR_EXCL_LINE

    gfx::Size gridCellSize()
    {
        return gfx::Size{
            .width = std::max(kWallTileSize.width, kFloorTileSize.width),
            .height =
                std::max(kWallTileSize.height, kFloorTileSize.height)};
    }

    std::optional<Tile> suggestedTileFor(
        const Tilemap &tilemap, const geometry::GridCell cell)
    {
        const auto wholeTilemap = defaultTilemap();
        const auto belongs = wholeTilemap.at(cell.column, cell.row);

        if (!belongs.has_value())
        {
            return std::nullopt;
        }

        if (!cellHoldingTile(tilemap, *belongs).has_value())
        {
            return belongs;
        }

        for (const auto tile : wholeTilemap.tiles)
        {
            if (tile.has_value() && tile->atlas == belongs->atlas
                && !cellHoldingTile(tilemap, *tile).has_value())
            {
                return tile;
            }
        }

        return std::nullopt;
    } // GCOVR_EXCL_LINE

    void putTile(
        Tilemap &tilemap, const geometry::GridCell cell, const Tile tile)
    {
        tilemap.tiles.at(
            (static_cast<std::size_t>(cell.row) * tilemap.columns)
            + cell.column) = tile;
    }

    void clearTile(Tilemap &tilemap, const geometry::GridCell cell)
    {
        tilemap.tiles.at(
            (static_cast<std::size_t>(cell.row) * tilemap.columns)
            + cell.column) = std::nullopt;
    }

    void swapTiles(
        Tilemap &tilemap,
        const geometry::GridCell fromCell,
        const geometry::GridCell toCell)
    {
        const auto tileAt = [&tilemap](const geometry::GridCell place)
        {
            return (static_cast<std::size_t>(place.row) * tilemap.columns)
                   + place.column;
        };

        std::swap(tilemap.tiles.at(tileAt(fromCell)),
            tilemap.tiles.at(tileAt(toCell)));
    }

    std::optional<geometry::GridCell> cellHoldingTile(
        const Tilemap &tilemap, const Tile tile)
    {
        for (std::uint32_t row = 0; row < tilemap.rows; ++row)
        {
            for (std::uint32_t column = 0; column < tilemap.columns;
                 ++column)
            {
                if (tilemap.at(column, row) == tile)
                {
                    return geometry::GridCell{
                        .column = column, .row = row};
                }
            }
        }

        return std::nullopt;
    }

}
