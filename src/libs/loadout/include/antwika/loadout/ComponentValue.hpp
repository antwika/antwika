#pragma once

#include <functional>
#include <map>
#include <string>
#include <variant>

#include <antwika/component/AnimationState.hpp>
#include <antwika/component/CarriedLight.hpp>
#include <antwika/component/FillLight.hpp>
#include <antwika/component/Health.hpp>
#include <antwika/component/Inventory.hpp>
#include <antwika/component/Patrol.hpp>
#include <antwika/component/Player.hpp>
#include <antwika/component/Position.hpp>
#include <antwika/component/CharacterIndex.hpp>
#include <antwika/component/Speaker.hpp>
#include <antwika/component/Velocity.hpp>

namespace antwika::loadout
{

    using ComponentValue = std::variant<
        component::Position,
        component::Velocity,
        component::AnimationState,
        component::Health,
        component::Inventory,
        component::Patrol,
        component::Speaker,
        component::Player,
        component::CharacterIndex,
        component::CarriedLight,
        component::FillLight>;

    using ComponentValues = std::map<std::string, ComponentValue, std::less<>>;

}
