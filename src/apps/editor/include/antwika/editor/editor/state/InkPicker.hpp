#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>
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

    struct InkPicker final
    {
        std::optional<std::size_t> editingInk;

        gfx::Color inkBeforeEditColor{};

        std::uint8_t glowBeforeEdit = 0;

        std::string hexText;

        Hsv pickerHsv{};

        bool pickerDragging = false;

        std::size_t activeInk = 0;

        std::array<std::vector<std::size_t>, 2> carriedInk{};

        std::vector<std::size_t> carriedCharacterInk{};

        std::vector<std::vector<std::size_t>> carriedFigureInk{};
    };

}
