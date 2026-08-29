#pragma once

#include <optional>
#include <vector>

#include <antwika/gfx/Math3D.hpp>
#include <antwika/voxel/VoxelPosition.hpp>

namespace antwika::gameplay
{

    class IWalkPath
    {
    public:
        IWalkPath() = default;

        virtual ~IWalkPath() = default;

        IWalkPath(const IWalkPath &) = delete;
        IWalkPath(IWalkPath &&) = delete;

        IWalkPath &operator=(const IWalkPath &) = delete;
        IWalkPath &operator=(IWalkPath &&) = delete;

        virtual void followPath(
            std::vector<gfx::Vec3> stopPositions,
            voxel::VoxelPosition goalPosition) = 0;

        [[nodiscard]] virtual const std::vector<gfx::Vec3> &getPath()
            const noexcept = 0;

        [[nodiscard]] virtual const std::optional<voxel::VoxelPosition> &
        getPathGoal() const noexcept = 0;

        virtual void stepAlongPath(bool playing) = 0;

        virtual void clearPath() noexcept = 0;
    };

}
