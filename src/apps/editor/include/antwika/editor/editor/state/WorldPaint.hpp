#pragma once

#include <optional>

#include <antwika/input/MouseButton.hpp>
#include <antwika/light/PointLight.hpp>
#include <antwika/voxel/VoxelPosition.hpp>

namespace antwika::editor
{

    struct WorldPaint final
    {
        std::optional<input::MouseButton> dragButton;

        std::optional<voxel::VoxelPosition> lastPaintedPosition;

        std::optional<voxel::VoxelPosition> shapeFromPosition;

        std::optional<light::Lamp> draggedLamp;
    };

}
