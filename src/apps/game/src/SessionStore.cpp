#include "antwika/game/SessionStore.hpp"

#include <array>
#include <cstddef>
#include <optional>

#include "antwika/game/Building.hpp"
#include "antwika/game/CityGrid.hpp"
#include "antwika/game/Coverage.hpp"
#include "antwika/game/Errand.hpp"
#include "antwika/game/Journey.hpp"
#include "antwika/game/Household.hpp"
#include "antwika/game/Production.hpp"
#include "antwika/game/Walker.hpp"
#include "antwika/game/Workforce.hpp"

namespace antwika::game
{

    SessionStore::SessionStore(
        World &world,
        PathIndex &paths,
        BuildingIndex &built,
        Camera &camera,
        GameState &state,
        GridExtent extent,
        std::uint64_t seed)
        : world(world),
          paths(paths),
          built(built),
          camera(camera),
          state(state),
          extent(extent),
          seed(seed)
    {
    }

    SaveGame SessionStore::take() const
    {
        return saveGameOf(world, paths, camera, state, extent, seed);
    }

    void SessionStore::restore(const SaveGame &save)
    {
        paths = pathIndexOf(save);
        camera = save.camera;
        state = save.state;

        // Translated rather than laid down here.
        // Putting entities on the live grid is one piece of code.
        // The city switch uses the very same one -- see CityGrid.
        // So the create-before-add rule is stated once.
        CityGrid grid;
        grid.walkers.reserve(save.walkers.size());
        grid.buildings.reserve(save.buildings.size());

        for (const auto &walker : save.walkers)
        {
            std::optional<Errand> errand;

            if (walker.errand.has_value())
            {
                errand = Errand{
                    .carrying = walker.errand->carrying,
                    .leg = walker.errand->leg};
            }

            std::optional<Journey> journey;

            if (walker.journey.has_value())
            {
                journey = Journey{.towards = walker.journey->towards};
            }

            grid.walkers.push_back(
                StoredWalker{
                    .at = walker.at,
                    .walker =
                        Walker{
                            .facing = walker.facing,
                            .kind = walker.kind,
                            .carried = walker.carried,
                            .stepsUntilHome = walker.stepsUntilHome,
                            .ticksUntilStep = walker.ticksUntilStep},
                    .home = walker.home,
                    .errand = errand,
                    .destination = walker.errand.has_value()
                        ? walker.errand->destination
                        : std::nullopt,
                    .journey = journey,
                    .joining = walker.journey.has_value()
                        ? walker.journey->house
                        : std::nullopt});
        }

        for (const auto &building : save.buildings)
        {
            // Slot by slot rather than by copying the list.
            // A file names as many walkers as the schema allows.
            // This is the one place that number becomes slots.
            std::array<std::optional<std::size_t>, kMaxWalkersOut> held{};

            for (std::size_t slot = 0; slot < kMaxWalkersOut; ++slot)
            {
                if (slot < building.walkers.size())
                {
                    held[slot] = building.walkers[slot];
                }
            }

            std::optional<Production> production;

            if (building.ticksUntilOutput.has_value())
            {
                production = Production{
                    .ticksUntilOutput = *building.ticksUntilOutput};
            }

            std::optional<Workforce> workforce;

            if (building.employed.has_value())
            {
                workforce = Workforce{.employed = *building.employed};
            }

            grid.buildings.push_back(
                StoredBuilding{
                    .at = building.at,
                    .building =
                        Building{
                            .kind = building.kind,
                            .stock = building.stock,
                            .risk = building.risk,
                            .ticksUntilSpawn = building.ticksUntilSpawn,
                            .ticksUntilDrain = building.ticksUntilDrain,
                            .ticksUntilRisk = building.ticksUntilRisk},
                    .walkers = held,
                    .coverage = Coverage{.ticksLeft = building.coverage},
                    .production = production,
                    .household = building.household,
                    .workforce = workforce});
        }

        restoreCityGrid(world, built, paths, grid);
    }

} // namespace antwika::game
