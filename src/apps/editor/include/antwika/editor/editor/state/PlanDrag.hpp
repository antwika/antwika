#pragma once

#include <cstddef>
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

    struct PlanDrag final
    {
        Column fromColumn = Column::Todo;

        std::size_t cardIndex = 0;

        gfx::Point grabbedAtPoint{};

        bool moved = false;

        std::optional<Column> overColumn;

        std::size_t dropIndex = 0;
    };

}
