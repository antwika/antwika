#pragma once

#include <antwika/ecs/World.hpp>

#include "antwika/game/BuildingIndex.hpp"
#include "antwika/game/Camera.hpp"
#include "antwika/game/PathIndex.hpp"

namespace antwika::game
{

    using antwika::ecs::World;

    struct LiveGrid final
    {
        World &world;

        PathIndex &paths;

        BuildingIndex &built;

        Camera &camera;
    };

}
