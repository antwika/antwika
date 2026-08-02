#include "antwika/game/LabourSystem.hpp"

#include <algorithm>
#include <cstdint>
#include <map>

#include <antwika/ecs/Entity.hpp>

#include "antwika/game/Building.hpp"
#include "antwika/game/Cell.hpp"
#include "antwika/game/Household.hpp"
#include "antwika/game/Workforce.hpp"

namespace antwika::game
{

    void LabourSystem::update(World &world, antwika::time::Tick)
    {
        using antwika::ecs::Entity;

        // Read straight off the view, since a sum is commutative.
        // So no order over the households is observable.
        std::int32_t workforce = 0;

        for (const auto entity : world.view<Household>())
        {
            workforce += world.get<Household>(entity).population;
        }

        // Collected into a map rather than shared out as the view walks.
        // A view's order is nobody's to name.
        // And this splits a limited amount -- see the class comment.
        // Cell alone is the key, with no tie-break at all.
        // Two buildings cannot share an origin -- see BuildingIndex.
        std::map<Cell, Entity> workplaces;

        for (const auto entity : world.view<Building, Cell>())
        {
            if (workersWantedBy(world.get<Building>(entity).kind) <= 0)
            {
                continue;
            }

            workplaces.emplace(world.get<Cell>(entity), entity);
        }

        std::int32_t left = workforce;

        for (const auto &[at, entity] : workplaces)
        {
            const auto wanted =
                workersWantedBy(world.get<Building>(entity).kind);
            const auto given = std::min(wanted, left);

            left -= given;

            setWorkforce(world, entity, Workforce{.employed = given});
        }
    }

} // namespace antwika::game
