#pragma once

#include <antwika/ecs/ISystem.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/intent/DirectionKeys.hpp>
#include <antwika/time/Tick.hpp>
#include <antwika/voxel/VoxelPosition.hpp>
#include <antwika/voxel/Voxels.hpp>
#include <antwika/collision/Collision.hpp>

namespace antwika::system
{

    class MoveIntentSystem final : public ecs::ISystem
    {
    public:
        MoveIntentSystem(
            const intent::DirectionKeys &wasdKeys,
            const intent::DirectionKeys &arrowKeys) noexcept;

        void setFrozen(bool frozen) noexcept;

        void setRunning(bool running) noexcept;

        void setSteering(
            float steeringX, float steeringZ) noexcept;

        void clearSteering() noexcept;

        void update(ecs::World &world, time::Tick tick) override;

    private:
        const intent::DirectionKeys *wasdKeys;
        const intent::DirectionKeys *arrowKeys;
        bool frozen = false;
        bool running = false;
        bool steering = false;
        float steerX = 0.0F;
        float steerZ = 0.0F;
    };

}
