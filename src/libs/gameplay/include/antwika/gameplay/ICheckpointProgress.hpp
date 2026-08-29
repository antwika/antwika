#pragma once

#include <string>

#include <antwika/gameplay/CheckpointState.hpp>
#include <antwika/map/MapFile.hpp>
#include <antwika/map/PlayerProgress.hpp>
#include <antwika/voxel/VoxelPosition.hpp>

namespace antwika::gameplay
{

    class ICheckpointProgress
    {
    public:
        ICheckpointProgress() = default;

        virtual ~ICheckpointProgress() = default;

        ICheckpointProgress(const ICheckpointProgress &) = delete;
        ICheckpointProgress(ICheckpointProgress &&) = delete;

        ICheckpointProgress &operator=(const ICheckpointProgress &)
            = delete;
        ICheckpointProgress &operator=(ICheckpointProgress &&) = delete;

        [[nodiscard]] virtual const CheckpointState &getCheckpoint()
            const noexcept = 0;

        virtual void setCheckpoint(CheckpointState checkpoint) noexcept
            = 0;

        [[nodiscard]] virtual map::Progress getProgress(
            std::string mapName) const = 0;
    };

}
