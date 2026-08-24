#include "antwika/editor/editor/state/SheetStroke.hpp"

#include <algorithm>
#include <functional>

#include "antwika/editor/ui/AtlasView.hpp"

namespace antwika::editor
{

    namespace
    {
        [[nodiscard]] bool holdsEveryEdge(
            const SheetStroke &sheetStroke,
            const std::function<bool(tilemap::TileEdge)> &edgeHolds)
        {
            if (!sheetStroke.selectedTile.has_value()
                || !sheetStroke.selectedEdges.has_value())
            {
                return false;
            }

            return std::ranges::all_of(
                edgesIn(*sheetStroke.selectedEdges), edgeHolds);
        }
    }

    bool SheetStroke::isForbidden(const tile::TileRules &rules) const
    {
        return holdsEveryEdge(
            *this,
            [this, &rules](const tilemap::TileEdge edge)
            { return rules.isForbidden(*selectedTile, edge); });
    }

    bool SheetStroke::allowsBoundary(const tile::TileRules &rules) const
    {
        return holdsEveryEdge(
            *this,
            [this, &rules](const tilemap::TileEdge edge)
            { return rules.allowsBoundary(*selectedTile, edge); });
    }

    bool SheetStroke::allows(
        const tile::TileRules &rules,
        const tilemap::Tile neighbourTile) const
    {
        return holdsEveryEdge(
            *this,
            [this, &rules, neighbourTile](const tilemap::TileEdge edge)
            { return rules.allows(*selectedTile, edge, neighbourTile); });
    }

}
