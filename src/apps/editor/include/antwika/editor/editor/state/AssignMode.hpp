#pragma once

#include <cstddef>
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

    struct AssignMode final
    {
        bool basePicking = false;

        std::size_t framePicked = 0;

        std::size_t memberPicked = 0;

        bool memberAssigning = false;

        std::size_t flipFramePicked = 0;

        bool flipFrameAssigning = false;

        bool variantPicking = false;
    };

}
