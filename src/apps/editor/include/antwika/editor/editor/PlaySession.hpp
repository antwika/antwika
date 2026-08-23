#pragma once

#include <vector>

#include <antwika/ecs/World.hpp>
#include <antwika/log/ILogger.hpp>
#include <antwika/voxel/VoxelPosition.hpp>
#include <antwika/voxelmap/Voxel.hpp>

#include "antwika/editor/editor/GameModule.hpp"

namespace antwika::editor
{

    class PlaySession final
    {
    public:
        PlaySession(
            log::ILogger &logger, const voxel::Voxels &solidVoxels);

        PlaySession(const PlaySession &) = delete;
        PlaySession(PlaySession &&) = delete;

        PlaySession &operator=(const PlaySession &) = delete;
        PlaySession &operator=(PlaySession &&) = delete;

        ~PlaySession() = default;

        std::vector<std::vector<voxel::VoxelPosition>> patrolPositions;

        ecs::World world;

        GameModule game;

        bool playing = false;

        bool titleScreenUp = false;
    };

}
