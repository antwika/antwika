#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

#include <antwika/geometry/Grid.hpp>
#include <antwika/gfx/PointF.hpp>
#include <antwika/gfx/RectF.hpp>
#include <antwika/gfx/Size.hpp>

#include "antwika/tilemap/Atlas.hpp"
#include "antwika/tilemap/Tile.hpp"

namespace antwika::tilemap
{

    struct Tilemap final
    {
        std::uint32_t columns = 0;

        std::uint32_t rows = 0;

        std::vector<std::optional<Tile>> tiles{};

        [[nodiscard]] bool isComplete() const;

        [[nodiscard]] std::optional<Tile> getEntryAt(
            std::uint32_t column, std::uint32_t row) const;

        [[nodiscard]] bool operator==(const Tilemap &other) const
            = default;
    };

    [[nodiscard]] gfx::Size tileSizeOf(Atlas atlas);

    [[nodiscard]] gfx::RectF getTileSource(Tile tile);

    [[nodiscard]] Tilemap getDefaultTilemap();

    [[nodiscard]] gfx::Size getGridCellSize();

    [[nodiscard]] std::optional<geometry::GridCell> getCellHoldingTile(
        const Tilemap &tilemap, Tile tile);

    [[nodiscard]] std::optional<Tile> suggestedTileFor(
        const Tilemap &tilemap, geometry::GridCell cell);

    void putTile(Tilemap &tilemap, geometry::GridCell cell, Tile tile);

    void clearTile(Tilemap &tilemap, geometry::GridCell cell);

    void swapTiles(
        Tilemap &tilemap,
        geometry::GridCell fromCell,
        geometry::GridCell toCell);

}
