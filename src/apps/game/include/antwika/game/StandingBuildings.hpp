#pragma once

#include <map>

#include <antwika/ecs/Entity.hpp>
#include <antwika/ecs/World.hpp>

#include "antwika/game/Cell.hpp"

namespace antwika::game
{

    using antwika::ecs::World;

    using StandingBuildings = std::map<Cell, antwika::ecs::Entity>;

    [[nodiscard]] StandingBuildings standingBuildings(const World &world);

}
