#pragma once

#include <optional>
#include <vector>

#include <antwika/ecs/World.hpp>
#include <antwika/component/DirectionKeys.hpp>
#include <antwika/log/ILogger.hpp>
#include <antwika/map/MapFile.hpp>
#include <antwika/system/SimulationState.hpp>
#include <antwika/voxel/VoxelPosition.hpp>
#include <antwika/voxelmap/Voxel.hpp>

#include "antwika/editor/editor/GameModule.hpp"

namespace antwika::editor
{

    class PlaySession final
    {
    public:
        PlaySession(
            log::ILogger &logger,
            const map::Map &laidMap,
            const voxel::Voxels &solidVoxels);

        PlaySession(const PlaySession &) = delete;
        PlaySession(PlaySession &&) = delete;

        PlaySession &operator=(const PlaySession &) = delete;
        PlaySession &operator=(PlaySession &&) = delete;

        ~PlaySession() = default;

        std::vector<std::vector<voxel::VoxelPosition>> patrolPositions;

        ecs::World world;

        GameModule game;

        system::SimulationState simulationState;

        component::DirectionKeys wasdKeys;

        component::DirectionKeys arrowKeys;

        bool playing = false;

        bool titleScreenUp = false;

        std::optional<map::Map> mapBeforePlay;

        bool wasDirtyBeforePlay = false;
    };

}
