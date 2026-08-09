#include "antwika/game/Demolition.hpp"

#include <algorithm>
#include <cstdint>
#include <optional>

#include <antwika/ecs/Entity.hpp>

#include "antwika/game/Building.hpp"
#include "antwika/game/Cell.hpp"
#include "antwika/game/Footprint.hpp"
#include "antwika/game/Household.hpp"
#include "antwika/game/HousingLevel.hpp"
#include "antwika/game/HousingQuery.hpp"
#include "antwika/game/Journey.hpp"
#include "antwika/game/Ruin.hpp"
#include "antwika/game/SpawnSystem.hpp"
#include "antwika/game/Walker.hpp"

namespace antwika::game
{

    namespace
    {
        using antwika::ecs::Entity;
        using antwika::ecs::kNullEntity;

        [[nodiscard]] std::int32_t roomIn(const World &world, Entity house)
        {
            if (house == kNullEntity)
            {
                return 0;
            }

            const auto household = householdOf(world, house);

            return std::max(
                populationCapacityOf(household.level)
                    - household.population,
                0);
        }

        [[nodiscard]] std::optional<Cell> escapeCellFor(
            Cell origin,
            Footprint footprint,
            const BuildingIndex &built,
            GridExtent extent)
        {
            std::optional<Cell> lowest;

            const auto consider = [&](Cell cell)
            {
                if (!extent.contains(cell) || built.has(cell))
                {
                    return;
                }

                if (!lowest.has_value() || cell < *lowest)
                {
                    lowest = cell;
                }
            };

            for (std::int32_t dy = -1; dy <= footprint.height; ++dy)
            {
                for (std::int32_t dx = -1; dx <= footprint.width; ++dx)
                {
                    if (dy > -1 && dy < footprint.height && dx > -1
                        && dx < footprint.width)
                    {
                        continue;
                    }

                    consider(
                        Cell{.x = origin.x + dx, .y = origin.y + dy});
                }
            }

            return lowest;
        }

        void turnOut(
            World &world,
            const BuildingIndex &built,
            Entity leaving,
            Cell from,
            std::int32_t people,
            GridExtent extent,
            const GameConfig &config)
        {
            const auto vacancy =
                nearestVacancy(world, from, leaving, built, extent);
            const auto gate = nearestGate(from, built, extent);

            auto beds = roomIn(world, vacancy);

            const auto out = world.view<Walker>().size();
            std::size_t sent = 0;

            for (std::int32_t person = 0; person < people; ++person)
            {
                if (out + sent >= config.walkerLimit)
                {
                    break;
                }

                const auto towards = beds > 0
                    ? std::optional<Cell>(world.get<Cell>(vacancy))
                    : gate;

                if (!towards.has_value())
                {
                    break;
                }

                const auto house = beds > 0 ? vacancy : kNullEntity;
                beds = std::max(beds - 1, 0);

                const auto leaver = world.create();
                world.add<Cell>(leaver, from);

                world.add<Walker>(
                    leaver,
                    Walker{.kind = WalkerKind::Migrant, .carried = 1});
                world.add<Journey>(
                    leaver, Journey{.towards = *towards, .house = house});

                ++sent;
            }
        }

        [[nodiscard]] std::int32_t occupantsOf(
            const World &world, Entity entity, const Building &building)
        {
            return housesPeople(building.kind)
                ? householdOf(world, entity).population
                : 0;
        }
    }

    void demolish(
        World &world,
        BuildingIndex &built,
        antwika::ecs::Entity entity,
        GridExtent extent,
        const GameConfig &config)
    {
        const auto at = world.get<Cell>(entity);
        const auto building = world.get<Building>(entity);
        const auto people = occupantsOf(world, entity, building);

        built.erase(at, footprintOf(building.kind));

        if (people > 0)
        {
            turnOut(world, built, entity, at, people, extent, config);
        }

        world.destroy(entity);
    }

    namespace
    {
        void fall(
            World &world,
            BuildingIndex &built,
            antwika::ecs::Entity entity,
            GridExtent extent,
            const GameConfig &config,
            RuinState state,
            std::int32_t ticksUntilOut)
        {
            const auto at = world.get<Cell>(entity);
            const auto building = world.get<Building>(entity);
            const auto people = occupantsOf(world, entity, building);

            const auto escape = people > 0
                ? escapeCellFor(
                      at, footprintOf(building.kind), built, extent)
                : std::nullopt;

            if (escape.has_value())
            {
                turnOut(world, built, entity, *escape, people, extent, config);
            }

            const auto ruin = world.create();
            world.add<Cell>(ruin, at);
            world.add<Ruin>(
                ruin,
                Ruin{
                    .kind = building.kind,
                    .state = state,
                    .ticksUntilOut = ticksUntilOut});

            world.destroy(entity);
        }
    }

    void ignite(
        World &world,
        BuildingIndex &built,
        antwika::ecs::Entity entity,
        GridExtent extent,
        const GameConfig &config)
    {
        fall(
            world,
            built,
            entity,
            extent,
            config,
            RuinState::Burning,
            config.burnDurationTicks);
    }

    void collapse(
        World &world,
        BuildingIndex &built,
        antwika::ecs::Entity entity,
        GridExtent extent,
        const GameConfig &config)
    {
        fall(world, built, entity, extent, config, RuinState::Debris, 0);
    }

}
