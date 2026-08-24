#pragma once

#include <vector>

#include <antwika/solver/TileSolve.hpp>
#include <antwika/tilemap/Tilemap.hpp>

namespace antwika::map
{

    struct WovenTiles final
    {
        std::vector<tilemap::Tile> tiles{};

        solver::TileSolve solve{};
    };

}
