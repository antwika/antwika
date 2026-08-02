#pragma once

#include <map>

#include <antwika/ecs/Entity.hpp>
#include <antwika/ecs/World.hpp>

#include "antwika/game/Cell.hpp"

namespace antwika::game
{

    using antwika::ecs::World;

    /**
     * @brief Which building stands on each cell of the grid.
     *
     * Keyed by **every** cell a block covers rather than by its origin,
     * so a walker beside a farm's far corner finds the farm; a lookup by
     * origin would need every caller to know each kind's footprint and
     * try each offset itself.
     *
     * Ordered by Cell, so anything that walks it walks it in a total
     * order somebody can name -- unlike ecs::View, whose order is
     * "whichever storage has the fewest entities".
     */
    using StandingBuildings = std::map<Cell, antwika::ecs::Entity>;

    /**
     * @brief Work out what is standing where.
     *
     * One statement of it, shared by everything that has to ask "is
     * there a building beside this walker": two copies of the loop would
     * be two places a footprint could be read differently.
     *
     * @param world Read for the buildings, as of its last commit().
     * @return One entry per cell any building stands on.
     */
    [[nodiscard]] StandingBuildings standingBuildings(const World &world);

} // namespace antwika::game
