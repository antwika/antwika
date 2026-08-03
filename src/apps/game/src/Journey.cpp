#include "antwika/game/Journey.hpp"

#include <cstdint>
#include <map>
#include <set>

#include "antwika/game/Building.hpp"
#include "antwika/game/BuildingKind.hpp"
#include "antwika/game/Footprint.hpp"
#include "antwika/game/Homing.hpp"
#include "antwika/game/Household.hpp"
#include "antwika/game/HousingQuery.hpp"

namespace antwika::game
{

    namespace
    {
        using antwika::ecs::Entity;
        using antwika::ecs::kNullEntity;
        using antwika::ecs::World;

        // One cell wide, since a road is what is being walked onto.
        constexpr Footprint kOneCell{.width = 1, .height = 1};

        // A cell on the outermost ring of the grid.
        // There is nothing beyond the extent.
        // So a road that reaches it is a road that leads out of town.
        [[nodiscard]] bool onTheEdge(Cell cell, GridExtent extent) noexcept
        {
            return extent.contains(cell)
                   && (cell.x == 0 || cell.y == 0
                       || cell.x == extent.width - 1
                       || cell.y == extent.height - 1);
        }
    } // namespace

    std::optional<Cell> nearestGate(
        Cell from, const PathIndex &paths, GridExtent extent)
    {
        // Ascending Cell, out of a set rather than off the index.
        // Which gate is chosen decides which roads a migrant walks.
        // So it may not depend on the order roads were laid in.
        std::set<Cell> gates;

        for (const auto road : paths.cells())
        {
            if (onTheEdge(road, extent))
            {
                gates.insert(road);
            }
        }

        std::optional<Cell> best;
        std::optional<std::int64_t> shortest;

        for (const auto gate : gates)
        {
            const auto cost =
                routeCost(from, gate, kOneCell, paths, extent);

            if (!cost.has_value())
            {
                continue;
            }

            // Strictly shorter, so a tie keeps the lower Cell.
            // Which is the one the set handed over first.
            if (!shortest.has_value() || *cost < *shortest)
            {
                shortest = cost;
                best = gate;
            }
        }

        return best;
    }

    Entity nearestVacancy(
        const World &world,
        Cell from,
        Entity leaving,
        const PathIndex &paths,
        GridExtent extent)
    {
        // Ascending Cell, for nearestGate()'s reason exactly.
        // BuildingIndex keeps two buildings off one origin cell.
        // So the cell alone is already a total order.
        std::map<Cell, Entity> candidates;

        for (const auto entity : world.view<Building, Cell>())
        {
            if (entity == leaving
                || !housesPeople(world.get<Building>(entity).kind))
            {
                continue;
            }

            const auto household = householdOf(world, entity);

            if (household.population
                >= populationCapacityOf(household.level))
            {
                continue;
            }

            candidates.emplace(world.get<Cell>(entity), entity);
        }

        Entity best = kNullEntity;
        std::optional<std::int64_t> shortest;

        for (const auto &[at, entity] : candidates)
        {
            const auto cost = routeCost(
                from,
                at,
                footprintOf(world.get<Building>(entity).kind),
                paths,
                extent);

            if (!cost.has_value())
            {
                continue;
            }

            if (!shortest.has_value() || *cost < *shortest)
            {
                shortest = cost;
                best = entity;
            }
        }

        return best;
    }

} // namespace antwika::game
