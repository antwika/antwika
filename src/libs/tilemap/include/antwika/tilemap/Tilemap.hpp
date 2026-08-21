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

        [[nodiscard]] std::optional<Tile> at(
            std::uint32_t column, std::uint32_t row) const;

        [[nodiscard]] bool operator==(const Tilemap &other) const
            = default;
    };

    [[nodiscard]] gfx::Size tileSizeOf(Atlas atlas);

    [[nodiscard]] gfx::RectF tileSource(Tile tile);

    [[nodiscard]] Tilemap defaultTilemap();

    [[nodiscard]] gfx::Size gridCellSize();

    [[nodiscard]] std::optional<geometry::GridCell> cellHoldingTile(
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
