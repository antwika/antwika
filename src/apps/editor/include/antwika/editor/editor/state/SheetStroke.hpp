#pragma once

#include <optional>

#include <antwika/geometry/GridCell.hpp>
#include <antwika/gfx/PointF.hpp>
#include <antwika/tile/TileRules.hpp>
#include <antwika/tilemap/TileEdges.hpp>
#include <antwika/tilemap/Tilemap.hpp>

#include "antwika/editor/ui/EdgeSelection.hpp"

namespace antwika::editor
{

    struct SheetStroke final
    {
        [[nodiscard]] bool isForbidden(const tile::TileRules &rules) const;

        [[nodiscard]] bool allowsBoundary(
            const tile::TileRules &rules) const;

        [[nodiscard]] bool allows(
            const tile::TileRules &rules,
            tilemap::Tile neighbourTile) const;

        std::optional<geometry::GridCell> dragFromCell;

        std::optional<gfx::PointF> dragFromPoint;

        std::optional<gfx::PointF> doubleClickAtPoint;

        std::optional<tilemap::Tile> selectedTile;

        std::optional<EdgeSelection> selectedEdges;

        bool active = false;

        bool erases = false;

        std::optional<geometry::GridCell> lineFromCell;

        std::optional<geometry::GridCell> brushAtCell;
    };

}
