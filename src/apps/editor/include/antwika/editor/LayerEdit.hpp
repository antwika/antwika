#pragma once

#include <cstddef>

#include <antwika/map/Layers.hpp>
#include <antwika/map/MapFile.hpp>
#include <antwika/tile/TileRules.hpp>
#include <antwika/tilemap/Tilemap.hpp>

#include "antwika/editor/editor/state/AssignMode.hpp"
#include "antwika/editor/editor/state/SheetStroke.hpp"

namespace antwika::editor
{

    [[nodiscard]] bool isDecorLayer(std::size_t chosenLayer);

    [[nodiscard]] tile::TileRules &getActiveRules(
        map::Map &drawnMap, std::size_t chosenLayer);

    [[nodiscard]] const tile::TileRules &getActiveRules(
        const map::Map &drawnMap, std::size_t chosenLayer);

    [[nodiscard]] tilemap::Tile getEditedTile(
        const map::Map &drawnMap,
        std::size_t chosenLayer,
        const SheetStroke &stroke,
        const AssignMode &assignMode);

    [[nodiscard]] bool isAdjoining(
        const tile::TileRules &rules,
        tilemap::Tile oneTile,
        tilemap::Tile otherTile);

}
