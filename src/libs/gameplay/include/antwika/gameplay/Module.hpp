#pragma once

#include <cstdint>
#include <set>
#include <vector>

#include <antwika/ecs/World.hpp>
#include <antwika/log/ILogger.hpp>
#include <antwika/map/MapFile.hpp>
#include <antwika/voxel/VoxelPosition.hpp>
#include <antwika/voxel/Voxels.hpp>

#include "antwika/gameplay/IGame.hpp"

extern "C" antwika::gameplay::IGame *antwikaGameCreate(
    antwika::log::ILogger *logger,
    antwika::ecs::World *world,
    const antwika::map::Map *laidMap,
    const antwika::voxel::Voxels *solidVoxels,
    const std::vector<std::vector<antwika::voxel::VoxelPosition>>
        *patrolPositions);

extern "C" void antwikaGameDestroy(antwika::gameplay::IGame *game);

extern "C" std::uint64_t antwikaGameStamp();
