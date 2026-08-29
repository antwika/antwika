#include "antwika/system/MoveIntentSystem.hpp"

#include <algorithm>

#include <antwika/component/Player.hpp>
#include <antwika/component/Velocity.hpp>

namespace antwika::system
{

    MoveIntentSystem::MoveIntentSystem(
        const component::DirectionKeys &wasdKeys,
        const component::DirectionKeys &arrowKeys,
        const SimulationState &simulation) noexcept
        : wasdKeys(&wasdKeys),
          arrowKeys(&arrowKeys),
          simulation(&simulation)
    {
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
            wasdKeys->getAxisX() + arrowKeys->getAxisX(), -1.0F, 1.0F);
        const auto keyedZ = std::clamp(
            wasdKeys->getAxisZ() + arrowKeys->getAxisZ(), -1.0F, 1.0F);
        const auto steeringOnly = steering && keyedX == 0.0F
                            && keyedZ == 0.0F;
        const auto byX = steeringOnly ? steerX : keyedX;
        const auto byZ = steeringOnly ? steerZ : keyedZ;
        const auto share = simulation->running
                               ? collision::kRunSpeedMultiplier
                               : 1.0F;

        for (const auto entity :
             world.view<component::Player, component::Velocity>())
        {
            world.set<component::Velocity>(
                entity,
                simulation->walkerHeld
                    ? component::Velocity{}
                    : component::Velocity{
                            .velocityX = byX,
                            .velocityZ = byZ,
                            .speedMultiplier = share});
        }
    }

}
