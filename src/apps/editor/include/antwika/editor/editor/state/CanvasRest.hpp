#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <antwika/character/Character.hpp>
#include <antwika/character/CharacterMarks.hpp>
#include <antwika/geometry/Grid.hpp>
#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/Math3D.hpp>
#include <antwika/gfx/Point.hpp>
#include <antwika/map/MapFile.hpp>
#include <antwika/voxelmap/Voxel.hpp>
#include <antwika/collision/Collision.hpp>
#include "antwika/editor/plan/PlanBoard.hpp"
#include "antwika/editor/ui/ColorPicker.hpp"

namespace antwika::editor
{

    struct CanvasRest final
    {
        std::optional<geometry::GridCell> tileCell;

        std::optional<std::size_t> face;

        std::uint32_t sinceTick = 0;
    };

}
