#pragma once

#include <antwika/ecs/World.hpp>

#include "antwika/game/BuildingIndex.hpp"
#include "antwika/game/GameConfig.hpp"
#include "antwika/game/GridExtent.hpp"

namespace antwika::game
{

    using antwika::ecs::World;

    void demolish(
        World &world,
        BuildingIndex &built,
        antwika::ecs::Entity entity,
        GridExtent extent,
        const GameConfig &config);

    void ignite(
        World &world,
        BuildingIndex &built,
        antwika::ecs::Entity entity,
        GridExtent extent,
        const GameConfig &config);

    void collapse(
        World &world,
        BuildingIndex &built,
        antwika::ecs::Entity entity,
        GridExtent extent,
        const GameConfig &config);

}
