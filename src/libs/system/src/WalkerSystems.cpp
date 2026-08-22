#include "antwika/system/WalkerSystems.hpp"

#include <algorithm>

#include <antwika/component/Player.hpp>
#include <antwika/component/Position.hpp>
#include <antwika/component/Velocity.hpp>

namespace antwika::system
{

    MoveIntentSystem::MoveIntentSystem(
        const input::DirectionKeys &wasdKeys,
        const input::DirectionKeys &arrowKeys) noexcept
        : wasdKeys(&wasdKeys), arrowKeys(&arrowKeys)
    {
    }

    void MoveIntentSystem::setFrozen(const bool value) noexcept
    {
        frozen = value;
    }

    void MoveIntentSystem::setRunning(const bool value) noexcept
    {
        running = value;
    }

    void MoveIntentSystem::setSteering(
        const float byX, const float byZ) noexcept
    {
        steering = true;
        steerX = byX;
        steerZ = byZ;
    }

    void MoveIntentSystem::clearSteering() noexcept
    {
        steering = false;
    }

    void MoveIntentSystem::update(ecs::World &world, time::Tick)
    {
        const auto keyedX = std::clamp(
            wasdKeys->axisX() + arrowKeys->axisX(), -1.0F, 1.0F);
        const auto keyedZ = std::clamp(
            wasdKeys->axisZ() + arrowKeys->axisZ(), -1.0F, 1.0F);
        const auto steeringOnly = steering && keyedX == 0.0F
                            && keyedZ == 0.0F;
        const auto byX = steeringOnly ? steerX : keyedX;
        const auto byZ = steeringOnly ? steerZ : keyedZ;
        const auto share = running ? collision::kRunSpeedMultiplier : 1.0F;

        for (const auto entity :
             world.view<component::Player, component::Velocity>())
        {
            world.set<component::Velocity>(
                entity,
                frozen ? component::Velocity{}
                      : component::Velocity{
                            .velocityX = byX,
                            .velocityZ = byZ,
                            .speedMultiplier = share});
        }
    }

    WalkSystem::WalkSystem(
        const voxel::Voxels &solidVoxels) noexcept
        : solidVoxels(&solidVoxels)
    {
    }

    void WalkSystem::update(ecs::World &world, time::Tick)
    {
        for (const auto entity :
             world.view<component::Position, component::Velocity>())
        {
            world.set<component::Position>(
                entity,
                collision::movedWithCollision(
                    *solidVoxels,
                    world.get<component::Position>(entity),
                    world.get<component::Velocity>(entity)));
        }
    }

}
