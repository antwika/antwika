#pragma once

#include <cstdint>
#include <antwika/geometry/Grid.hpp>
#include <antwika/gfx/PointF.hpp>
#include <antwika/gfx/RectF.hpp>
#include <antwika/gfx/Size.hpp>

namespace antwika::tilemap
{

    enum class Atlas : std::uint8_t
    {
        Wall,
        Floor,
    };

}
