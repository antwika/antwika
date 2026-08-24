#include "antwika/editor/LayerEdit.hpp"

#include <antwika/decor/Decor.hpp>
#include <antwika/tilemap/TileEdges.hpp>
#include <antwika/voxel/VoxelCube.hpp>

namespace antwika::editor
{

    bool isDecorLayer(const std::size_t chosenLayer)
    {
        return chosenLayer != map::kBaseLayer;
    }

    tile::TileRules &getActiveRules(
        map::Map &drawnMap, const std::size_t chosenLayer)
    {
        return isDecorLayer(chosenLayer) ? drawnMap.decorRules
                                         : drawnMap.rules;
    }

    const tile::TileRules &getActiveRules(
        const map::Map &drawnMap, const std::size_t chosenLayer)
    {
        return isDecorLayer(chosenLayer) ? drawnMap.decorRules
                                         : drawnMap.rules;
    }

    tilemap::Tile getEditedTile(
        const map::Map &drawnMap,
        const std::size_t chosenLayer,
        const SheetStroke &stroke,
        const AssignMode &assignMode)
    {
        if (!isDecorLayer(chosenLayer) || !stroke.selectedTile.has_value()
            || assignMode.framePicked == 0)
        {
            return *stroke.selectedTile;
        }

        const auto *decor =
            decor::decorOf(drawnMap.decor, *stroke.selectedTile);

        return decor != nullptr
                       && assignMode.framePicked < decor->frameTiles.size()
                   ? decor->frameTiles.at(assignMode.framePicked)
                   : *stroke.selectedTile;
    } // GCOVR_EXCL_LINE

    bool isAdjoining(
        const tile::TileRules &rules,
        const tilemap::Tile oneTile,
        const tilemap::Tile otherTile)
    {
        for (const auto edge : tilemap::kEveryTileEdge)
        {
            if (decor::tilesCompatible(rules, oneTile, edge, otherTile)
                && decor::tilesCompatible(
                    rules, otherTile, voxel::getFacing(edge), oneTile))
            {
                return true;
            }
        }

        return false;
    }

}
