#pragma once

#include <antwika/ecs/Entity.hpp>
#include <antwika/ecs/World.hpp>

#include "antwika/gameplay/ICheckpointProgress.hpp"

namespace antwika::gameplay
{

    /**
     * @brief Moves the respawn to the checkpoint the walker reported,
     * taking the report away, and says whether the respawn moved.
     */
    [[nodiscard]] bool takeCheckpointReport(
        ICheckpointProgress &checkpointProgress,
        ecs::World &world,
        ecs::Entity walkerEntity);

    /**
     * @brief Whether the walker reported reaching the exit, taking the
     * report away as it answers.
     */
    [[nodiscard]] bool takeExitReport(
        ecs::World &world, ecs::Entity walkerEntity);

}
