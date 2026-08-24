#pragma once

#include <cstddef>
#include <optional>

#include <antwika/tilemap/Tilemap.hpp>

namespace antwika::editor
{

    struct TransitionPick final
    {
        std::optional<tilemap::Tile> fromTile;

        std::optional<tilemap::Tile> toTile;

        std::optional<std::size_t> chosenIndex;
    };

}
