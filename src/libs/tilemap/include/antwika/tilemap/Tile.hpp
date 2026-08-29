#pragma once

#include <cstdint>

#include <antwika/geometry/Grid.hpp>
#include <antwika/gfx/PointF.hpp>
#include <antwika/gfx/RectF.hpp>
#include <antwika/gfx/Size.hpp>

#include "antwika/tilemap/Atlas.hpp"

namespace antwika::tilemap
{

    struct Tile final
    {
        Atlas atlas = Atlas::Wall;

        std::uint16_t index = 0;

        [[nodiscard]] bool operator==(const Tile &other) const
            = default;

        [[nodiscard]] auto operator<=>(const Tile &other) const
            = default;
    };

}
