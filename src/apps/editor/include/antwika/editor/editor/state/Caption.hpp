#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
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

    struct Caption final
    {
        std::string name;

        std::string line;

        std::optional<std::size_t> speaker;

        std::uint32_t start = 0;

        std::uint32_t untilTick = 0;
    };

}
