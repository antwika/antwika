#pragma once

#include <cstdint>
#include <set>
#include <vector>

#include <antwika/ecs/World.hpp>
#include <antwika/log/ILogger.hpp>
#include <antwika/voxel/VoxelCell.hpp>

#include "antwika/gameplay/IGame.hpp"

extern "C" antwika::gameplay::IGame *antwikaGameCreate(
    antwika::log::ILogger *logger,
    antwika::ecs::World *world,
    const std::set<antwika::voxel::VoxelCell> *solidCells,
    const std::vector<std::vector<antwika::voxel::VoxelCell>>
        *patrolCells);

extern "C" void antwikaGameDestroy(antwika::gameplay::IGame *game);

extern "C" std::uint64_t antwikaGameStamp();
