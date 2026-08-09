#pragma once

#include <antwika/ecs/Entity.hpp>
#include <antwika/ecs/World.hpp>

#include "antwika/game/Cell.hpp"
#include "antwika/game/GridExtent.hpp"
#include "antwika/game/PathIndex.hpp"
#include "antwika/game/Resource.hpp"

namespace antwika::game
{

    [[nodiscard]] antwika::ecs::Entity nearestAccepting(
        const antwika::ecs::World &world,
        Cell from,
        Resource resource,
        const PathIndex &paths,
        GridExtent extent);

    [[nodiscard]] antwika::ecs::Entity nearestHolding(
        const antwika::ecs::World &world,
        Cell from,
        Resource resource,
        const PathIndex &paths,
        GridExtent extent);

}
