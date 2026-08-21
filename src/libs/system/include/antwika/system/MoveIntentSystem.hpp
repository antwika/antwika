#pragma once

#include <antwika/ecs/ISystem.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/input/DirectionKeys.hpp>
#include <antwika/time/Tick.hpp>
#include <antwika/voxel/VoxelCell.hpp>
#include <antwika/collision/Collision.hpp>

namespace antwika::system
{

    class MoveIntentSystem final : public ecs::ISystem
    {
    public:
        MoveIntentSystem(
            const input::DirectionKeys &wasdKeys,
            const input::DirectionKeys &arrowKeys) noexcept;

        void setFrozen(bool frozen) noexcept;

        void setRunning(bool running) noexcept;

        void setSteering(
            float steeringX, float steeringZ) noexcept;

        void clearSteering() noexcept;

        void update(ecs::World &world, time::Tick tick) override;

    private:
        const input::DirectionKeys *wasdKeys;
        const input::DirectionKeys *arrowKeys;
        bool frozen = false;
        bool running = false;
        bool steering = false;
        float steerX = 0.0F;
        float steerZ = 0.0F;
    };

}
