#pragma once

#include <antwika/ecs/ISystem.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/component/DirectionKeys.hpp>
#include <antwika/time/Tick.hpp>
#include <antwika/voxel/VoxelPosition.hpp>
#include <antwika/voxel/Voxels.hpp>
#include <antwika/collision/Collision.hpp>

#include "antwika/system/SimulationState.hpp"

namespace antwika::system
{

    class MoveIntentSystem final : public ecs::ISystem
    {
    public:
        MoveIntentSystem(
            const component::DirectionKeys &wasdKeys,
            const component::DirectionKeys &arrowKeys,
            const SimulationState &simulation) noexcept;

        void setSteering(
            float steeringX, float steeringZ) noexcept;

        void clearSteering() noexcept;

        void update(ecs::World &world, time::Tick tick) override;

    private:
        const component::DirectionKeys *wasdKeys;
        const component::DirectionKeys *arrowKeys;
        const SimulationState *simulation;
        bool steering = false;
        float steerX = 0.0F;
        float steerZ = 0.0F;
    };

}
