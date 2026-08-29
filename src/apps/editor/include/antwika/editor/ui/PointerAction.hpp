#pragma once

#include <cstdint>

#include <antwika/enums/Enumeration.hpp>
#include <antwika/gfx/PointF.hpp>
#include <antwika/gfx/RectF.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/input/Key.hpp>
#include <antwika/tilemap/TileEdges.hpp>
#include <antwika/tilemap/Tilemap.hpp>
#include <antwika/widget/WidgetId.hpp>
#include <antwika/voxel/VoxelCube.hpp>

namespace antwika::editor
{

    enum class PointerAction : std::uint8_t
    {
        Nothing,
        Swap,
        Look,
        Rule,
        PixelSelection,
        Turn,
    };

}
