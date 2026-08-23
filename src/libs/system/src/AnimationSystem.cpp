#include "antwika/system/AnimationSystem.hpp"

#include <cstdint>

#include <antwika/character/Character.hpp>
#include <antwika/component/AnimationState.hpp>
#include <antwika/component/Velocity.hpp>
#include <antwika/collision/Collision.hpp>

namespace antwika::system
{

    void AnimationSystem::update(
        ecs::World &world, const time::Tick tick)
    {
        using component::AnimationState;

        for (const auto entity :
             world.view<component::Velocity, AnimationState>())
        {
            const auto was = world.get<AnimationState>(entity);
            const auto direction = character::getFacingFromVelocity(
                world.get<component::Velocity>(entity));
            const auto facing =
                direction.has_value()
                    ? static_cast<std::uint8_t>(*direction)
                    : was.direction;
            const auto walking = direction.has_value();
            const auto sameState = walking == was.walking
                                   && facing == was.direction;

            world.set<AnimationState>(
                entity,
                AnimationState{
                    .direction = facing,
                    .walking = walking,
                    .startedAtTick =
                        sameState ? was.startedAtTick : tick});
        }
    }

}
