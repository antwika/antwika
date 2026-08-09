#pragma once

#include <array>
#include <string>

#include "antwika/game/TileAtlas.hpp"

namespace antwika::game
{

    struct AtlasAssets final
    {
        std::array<std::string, kAtlasKindCount> byKind{};

        std::string walker;

        [[nodiscard]] bool operator==(const AtlasAssets &other) const =
            default;
    };

}
