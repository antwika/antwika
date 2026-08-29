#pragma once

#include <antwika/ecs/Entity.hpp>
#include <antwika/ecs/ISystem.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/map/MapFile.hpp>
#include <antwika/time/Tick.hpp>
#include <antwika/voxel/Voxels.hpp>

#include "antwika/gameplay/ICheckpointProgress.hpp"

namespace antwika::gameplay
{

    class SpawnSystem final : public ecs::ISystem
    {
    public:
        SpawnSystem(
            const map::Map &laidMap,
            const voxel::Voxels &solidVoxels,
            const ICheckpointProgress &checkpointProgress) noexcept;

        /**
         * @brief Stands the hero on the start pad, or on the checkpoint it
         * last reached, whenever the world holds no walker to play as.
         */
        void update(ecs::World &world, time::Tick tick) override;

        [[nodiscard]] ecs::Entity getStoodWalkerEntity() const noexcept;

    private:
        const map::Map *laidMap;
        const voxel::Voxels *solidVoxels;
        const ICheckpointProgress *checkpointProgress;
        ecs::Entity stoodWalkerEntity{};
    };

}
