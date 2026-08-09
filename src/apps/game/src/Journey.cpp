#include "antwika/game/Journey.hpp"

#include <cstdint>
#include <cstdlib>
#include <map>
#include <set>
#include <utility>

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

        constexpr Footprint kOneCell{.width = 1, .height = 1};

        [[nodiscard]] bool onTheEdge(Cell cell, GridExtent extent) noexcept
        {
            return cell.x == 0 || cell.y == 0
                   || cell.x == extent.width - 1
                   || cell.y == extent.height - 1;
        }

        [[nodiscard]] std::int64_t apartFrom(Cell one, Cell other) noexcept
        {
            return std::abs(static_cast<std::int64_t>(one.x) - other.x)
                   + std::abs(
                       static_cast<std::int64_t>(one.y) - other.y);
        }
    }

    std::optional<Cell> nearestGate(
        Cell from, const BuildingIndex &built, GridExtent extent)
    {
        std::set<std::pair<std::int64_t, Cell>> gates;

        for (std::int32_t x = 0; x < extent.width; ++x)
        {
            for (std::int32_t y = 0; y < extent.height; ++y)
            {
                const Cell cell{.x = x, .y = y};

                if (!onTheEdge(cell, extent) || built.has(cell))
                {
                    continue;
                }

                gates.emplace(apartFrom(from, cell), cell);
            }
        }

        for (const auto &[away, gate] : gates)
        {
            if (crossingCost(from, gate, kOneCell, built, extent)
                    .has_value())
            {
                return gate;
            }
        }

        return std::nullopt;
    }

    Entity nearestVacancy(
        const World &world,
        Cell from,
        Entity leaving,
        const BuildingIndex &built,
        GridExtent extent)
    {
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
            const auto cost = crossingCost(
                from,
                at,
                footprintOf(world.get<Building>(entity).kind),
                built,
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

}
