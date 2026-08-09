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
#include "antwika/game/Ruin.hpp"
#include "antwika/game/Walker.hpp"

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

        CityGrid grid;
        grid.walkers.reserve(save.walkers.size());
        grid.buildings.reserve(save.buildings.size());
        grid.ruins.reserve(save.ruins.size());

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
                        : std::nullopt,
                    .attending = walker.fireCall});
        }

        for (const auto &ruin : save.ruins)
        {
            grid.ruins.push_back(
                StoredRuin{
                    .at = ruin.at,
                    .ruin = Ruin{
                        .kind = ruin.kind,
                        .state = ruin.state,
                        .ticksUntilOut = ruin.ticksUntilOut}});
        }

        for (const auto &building : save.buildings)
        {
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

            // GCOVR_EXCL_START
            grid.buildings.push_back(
                StoredBuilding{
                    .at = building.at,
                    .building =
                        Building{
                            .kind = building.kind,
                            .stock = building.stock,
                            .fireRisk = building.risk,
                            .collapseRisk = building.collapseRisk,
                            .diseaseRisk = building.diseaseRisk,
                            .ticksUntilSpawn = building.ticksUntilSpawn,
                            .ticksUntilDrain = building.ticksUntilDrain,
                            .ticksUntilRisk = building.ticksUntilRisk,
                            .selling = building.selling},
                    .walkers = held,
                    .coverage = Coverage{.ticksLeft = building.coverage},
                    .production = production,
                    .household = building.household,
                    .staff = building.staff,
                    .employment = building.employment});
            // GCOVR_EXCL_STOP
        }

        restoreCityGrid(world, built, paths, grid);
    }

}
