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
    } // namespace

    void demolish(
        World &world,
        BuildingIndex &built,
        antwika::ecs::Entity entity,
        GridExtent extent)
    {
        const auto at = world.get<Cell>(entity);
        const auto building = world.get<Building>(entity);

        const auto people = housesPeople(building.kind)
            ? householdOf(world, entity).population
            : 0;

        // Out of the index before anybody routes anywhere.
        // The leavers start on this very ground.
        // A search still counting the block would find no way off it.
        built.erase(at, footprintOf(building.kind));

        // Asked once: the world does not change under this loop.
        // The building coming down is never the answer.
        // It may well have room, and would take its people straight back.
        const auto vacancy =
            people > 0 ? nearestVacancy(world, at, entity, built, extent)
                       : kNullEntity;
        const auto gate =
            people > 0 ? nearestGate(at, built, extent) : std::nullopt;

        auto beds = roomIn(world, vacancy);

        // Committed walkers only, since staged ones are invisible.
        // So the ones made below are counted by hand instead.
        const auto out = world.view<Walker>().size();
        std::size_t sent = 0;

        for (std::int32_t person = 0; person < people; ++person)
        {
            if (out + sent >= kWalkerLimit)
            {
                break;
            }

            // A spare bed in town before the road out of it.
            const auto towards = beds > 0
                ? std::optional<Cell>(world.get<Cell>(vacancy))
                : gate;

            // Nowhere at all to walk to, so this person is gone.
            // The rule a walled-in house already lives under.
            if (!towards.has_value())
            {
                break;
            }

            const auto house = beds > 0 ? vacancy : kNullEntity;
            beds = std::max(beds - 1, 0);

            const auto leaver = world.create();
            world.add<Cell>(leaver, at);

            // Nobody's walker: the house they lived in is coming down.
            world.add<Walker>(leaver, Walker{.kind = WalkerKind::Migrant});
            world.add<Journey>(
                leaver, Journey{.towards = *towards, .house = house});

            ++sent;
        }

        world.destroy(entity);
    }

} // namespace antwika::game
