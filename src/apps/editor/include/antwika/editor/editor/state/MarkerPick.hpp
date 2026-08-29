#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>

#include <antwika/map/Marker.hpp>
#include <antwika/voxel/VoxelCube.hpp>
#include <antwika/voxel/VoxelPosition.hpp>

#include "antwika/editor/Preferences.hpp"

namespace antwika::editor
{

    struct MarkerPick final
    {
        std::optional<map::Marker> marker;

        voxel::VoxelPosition position{};

        std::optional<std::size_t> editingAxis;

        std::string pendingAxisText;
    };

    [[nodiscard]] constexpr std::optional<map::Marker> getMarkerOf(
        const Tool tool) noexcept
    {
        switch (tool)
        {
        case Tool::Checkpoint:
            return map::Marker::Checkpoint;
        case Tool::Food:
            return map::Marker::Food;
        case Tool::Water:
            return map::Marker::Water;
        default:
            break;
        }

        return std::nullopt;
    }

    [[nodiscard]] constexpr std::int32_t getAxisOf(
        const voxel::VoxelPosition position, const std::size_t axis) noexcept
    {
        return axis == 0 ? position.x
               : axis == 1 ? position.y
                           : position.z;
    }

    [[nodiscard]] constexpr voxel::VoxelPosition getWithAxisSet(
        voxel::VoxelPosition position,
        const std::size_t axis,
        const std::int32_t value) noexcept
    {
        (axis == 0 ? position.x
         : axis == 1 ? position.y
                     : position.z) = value;

        return position;
    }

    /**
     * @brief One axis of the cube a position stands in, which is the count
     * the editor shows and takes.
     */
    [[nodiscard]] inline std::int32_t getCubeAxisOf(
        const voxel::VoxelPosition position, const std::size_t axis)
    {
        return getAxisOf(voxel::cubeIndexOf(position), axis);
    }

    /**
     * @brief The position moved to the named cube along one axis, keeping
     * where it stands within its own cube.
     */
    [[nodiscard]] inline voxel::VoxelPosition getWithCubeAxisSet(
        const voxel::VoxelPosition position,
        const std::size_t axis,
        const std::int32_t cubeValue)
    {
        const auto withinCube =
            getAxisOf(position, axis)
            - getAxisOf(voxel::cubeCornerOf(position), axis);

        return getWithAxisSet(
            position, axis, (cubeValue * voxel::kCubeSide) + withinCube);
    }

}
