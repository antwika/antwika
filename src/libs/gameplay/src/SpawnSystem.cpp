#include "antwika/gameplay/SpawnSystem.hpp"

#include <antwika/component/Player.hpp>
#include <antwika/ecs/OpenPhase.hpp>
#include <antwika/map/MapFile.hpp>

#include "antwika/gameplay/Characters.hpp"

namespace antwika::gameplay
{

    SpawnSystem::SpawnSystem(
        const map::Map &laidMap,
        const voxel::Voxels &solidVoxels,
        const ICheckpointProgress &checkpointProgress) noexcept
        : laidMap(&laidMap),
          solidVoxels(&solidVoxels),
          checkpointProgress(&checkpointProgress)
    {
    }

    void SpawnSystem::update(ecs::World &world, time::Tick)
    {
        for (const auto entity : world.view<component::Player>())
        {
            stoodWalkerEntity = entity;

            return;
        }

        const auto hero = map::getPlayerIndex(*laidMap);

        if (!hero.has_value())
        {
            return;
        }

        {
            const ecs::OpenPhase phase(world);

            stoodWalkerEntity = spawnWalker(
                world,
                *laidMap,
                *hero,
                getStartingPlacement(
                    world,
                    *laidMap,
                    *solidVoxels,
                    checkpointProgress->getCheckpoint().placement));
        }

        requireOneSteerPerWalker(world);
    }

    ecs::Entity SpawnSystem::getStoodWalkerEntity() const noexcept
    {
        return stoodWalkerEntity;
    }

}
