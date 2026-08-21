#include "antwika/gameplay/Module.hpp"

#include "antwika/gameplay/SeamStamp.hpp"
#include "antwika/gameplay/Game.hpp"

extern "C" antwika::gameplay::IGame *antwikaGameCreate(
    antwika::log::ILogger *const logger,
    antwika::ecs::World *const world,
    const std::set<antwika::voxel::VoxelCell> *const solids,
    const std::vector<std::vector<antwika::voxel::VoxelCell>> *const
        patrolStops)
{
    return new antwika::gameplay::Game(
        *logger, *world, *solids, *patrolStops);
}

extern "C" void antwikaGameDestroy(antwika::gameplay::IGame *const game)
{
    delete game;
}

extern "C" std::uint64_t antwikaGameStamp()
{
    return antwika::gameplay::kSeamStamp;
}
