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

        // How many more people the nearest vacancy can actually take.
        // Counted here so the overflow heads for the gate at once.
        // Rather than crowding a doorway that clamps them to nothing.
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

        // The lowest free cell round a block's own perimeter.
        // Where the leavers of a burning building step out.
        // demolish() needs no such cell: it frees the ground first.
        // ignite() leaves the block standing.
        // So its people cannot spawn on it.
        // A route is re-searched every step.
        // And one starting on a covered cell finds no way off it.
        // Ascending Cell order, so a replay and a restore agree.
        // A road is as good as a field here; only a block refuses.
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
                    // The ring alone; the block itself is never free.
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

        // The people rule demolish() and ignite() share.
        // The nearest vacancy is asked once and its beds are counted.
        // That many head there and the rest make for the gate.
        // Whoever exceeds the walker limit is gone.
        // So is whoever has nowhere at all to walk to.
        // That is the rule a walled-in house already lives under.
        void turnOut(
            World &world,
            const BuildingIndex &built,
            Entity leaving,
            Cell from,
            std::int32_t people,
            GridExtent extent,
            const GameConfig &config)
        {
            // Asked once: the world does not change under this loop.
            // The building coming down is never the answer.
            // It may well have room, and would take its people back.
            const auto vacancy =
                nearestVacancy(world, from, leaving, built, extent);
            const auto gate = nearestGate(from, built, extent);

            auto beds = roomIn(world, vacancy);

            // Committed walkers only, since staged ones are invisible.
            // So the ones made below are counted by hand instead.
            const auto out = world.view<Walker>().size();
            std::size_t sent = 0;

            for (std::int32_t person = 0; person < people; ++person)
            {
                if (out + sent >= config.walkerLimit)
                {
                    break;
                }

                // A spare bed in town before the road out of it.
                const auto towards = beds > 0
                    ? std::optional<Cell>(world.get<Cell>(vacancy))
                    : gate;

                // Nowhere at all to walk to, so this person is gone.
                if (!towards.has_value())
                {
                    break;
                }

                const auto house = beds > 0 ? vacancy : kNullEntity;
                beds = std::max(beds - 1, 0);

                const auto leaver = world.create();
                world.add<Cell>(leaver, from);

                // Nobody's walker: the home they had is coming down.
                world.add<Walker>(
                    leaver, Walker{.kind = WalkerKind::Migrant});
                world.add<Journey>(
                    leaver, Journey{.towards = *towards, .house = house});

                ++sent;
            }
        }

        // Who lives in a building about to come down, if anybody.
        [[nodiscard]] std::int32_t occupantsOf(
            const World &world, Entity entity, const Building &building)
        {
            return housesPeople(building.kind)
                ? householdOf(world, entity).population
                : 0;
        }
    } // namespace

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

        // Out of the index before anybody routes anywhere.
        // The leavers start on this very ground.
        // A search still counting the block would find no way off it.
        built.erase(at, footprintOf(building.kind));

        if (people > 0)
        {
            turnOut(world, built, entity, at, people, extent, config);
        }

        world.destroy(entity);
    }

    namespace
    {
        // The ending ignite() and collapse() share, whole.
        // The block stays in the index: a ruin is not bare ground.
        // So the leavers step out at the perimeter instead.
        // A building walled in turns its people out to nowhere.
        // Which is the walled-in house's own rule.
        // What differs between the two callers is only what stands up.
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

            // The ruin stands up where the building stood.
            // A fresh entity rather than a reused one.
            // So every system holding a handle reads the building dead.
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
    } // namespace

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
        // Straight to debris: there is no fire to put out.
        // So no fireman is ever called to a collapse.
        fall(world, built, entity, extent, config, RuinState::Debris, 0);
    }

} // namespace antwika::game
