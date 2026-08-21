#include "antwika/editor/editor/PlayComponents.hpp"

#include <antwika/component/AnimationState.hpp>
#include <antwika/component/CarriedLight.hpp>
#include <antwika/component/FillLight.hpp>
#include <antwika/component/Health.hpp>
#include <antwika/component/Inventory.hpp>
#include <antwika/component/Item.hpp>
#include <antwika/component/Orientation.hpp>
#include <antwika/component/Patrol.hpp>
#include <antwika/component/Player.hpp>
#include <antwika/component/Position.hpp>
#include <antwika/component/RosterIndex.hpp>
#include <antwika/component/Speaker.hpp>
#include <antwika/component/Velocity.hpp>

namespace antwika::editor
{

    void claimPlayComponents(ecs::World &world)
    {
        world.claim<component::Player>();
        world.claim<component::Position>();
        world.claim<component::Velocity>();
        world.claim<component::AnimationState>();
        world.claim<component::RosterIndex>();
        world.claim<component::Speaker>();
        world.claim<component::CarriedLight>();
        world.claim<component::FillLight>();
        world.claim<component::Health>();
        world.claim<component::Inventory>();
        world.claim<component::Item>();
        world.claim<component::Orientation>();
        world.claim<component::Patrol>();
    }

}
