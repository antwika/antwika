#pragma once

#include <array>
#include <cstddef>

#include "antwika/game/Camera.hpp"
#include "antwika/game/CityGrid.hpp"
#include "antwika/game/LiveGrid.hpp"
#include "antwika/game/PathIndex.hpp"
#include "antwika/game/WorldMap.hpp"

namespace antwika::game
{

    /**
     * @brief Which city is open, and what has been built on each city's
     * grid.
     *
     * **Simulation state, not render state**, for exactly the reason
     * Camera is: a click arrives as a pixel, and what that pixel means
     * depends entirely on which map is showing. A view owned by the
     * renderer would leave a replay resolving a recorded click against
     * whichever map it happened to be on -- still deterministically,
     * just deterministically wrong. So this is folded from replayable
     * input like anything else, and the renderer only reads it.
     *
     * Which *screen* is up is AppMode's answer rather than a MapView of
     * this class's own. There used to be both, and two enumerations
     * saying the same thing are two truths to keep in step: a sink that
     * requested AppMode::WorldMap and forgot to close the city here
     * would leave the two disagreeing, and nothing would say so. What
     * is left here is the question AppMode cannot answer -- *which*
     * city, and what is on its grid.
     *
     * The world itself is generated from an integer seed held in
     * WorldMapConfig, so what a replay carries is the seed rather than
     * the map, and the map comes back identical.
     *
     * Each city keeps its own PathIndex, its own Camera and its own
     * CityGrid, so leaving a city and coming back shows what was built
     * there and shows it from where it was left. They live here rather
     * than in one shared grid because "the grid" is per city: two cities
     * that shared a PathIndex would show each other's roads, and two
     * that shared a CityGrid would show each other's buildings and
     * walkers.
     *
     * **A session still builds on one LiveGrid**, which openCityAt() and
     * closeCity() swap these in and out of. That is what keeps every
     * collaborator that builds, walks or draws the grid holding one
     * reference rather than resolving a city index on every call -- and
     * it is what a save file already assumes, since SaveGame carries one
     * grid.
     *
     * The entities are the part that cannot simply be assigned across:
     * there is one ecs::World, which is neither copyable nor movable, so
     * a closed city keeps its walkers and its buildings as values and
     * they are destroyed and recreated as cities are swapped. That is
     * also what makes a stale handle safe here rather than merely
     * unlikely: a stored city holds no ecs::Entity at all, and a reused
     * index comes back with its generation bumped, so nothing a switch
     * leaves behind can name a *different* entity later -- only a dead
     * one, which is the state Building::walker already treats as
     * authority.
     */
    class WorldMapState final
    {
    public:
        /**
         * @brief Construct the state showing a freshly generated
         * world.
         * @param world The generated world; its four cities are the
         * ones this can open.
         */
        explicit WorldMapState(WorldMap world);

        /**
         * @brief Get the world being played on.
         * @return The map, its terrain and its cities.
         */
        [[nodiscard]] const WorldMap &world() const noexcept;

        /**
         * @brief Check whether a city's grid is the live one.
         * @return True until closeCity() puts it away. A freshly
         * constructed state has city 0 open, so a run that never opens
         * the world map builds on one grid exactly as it always did.
         */
        [[nodiscard]] bool cityOpen() const noexcept;

        /**
         * @brief Get which city the live grid belongs to.
         * @return The last opened city's index, zero until one is.
         * Always a real index, so a renderer needs no fallback for the
         * tick between a city being put away and the world map coming
         * up.
         */
        [[nodiscard]] std::size_t city() const noexcept;

        /**
         * @brief Open a city, swapping its grid in as the live one.
         *
         * Whatever is on the live grid is put away with the city that
         * is open first, so opening the one that is already open is a
         * no-op rather than a way of losing what is on it.
         *
         * @param index An index below kCityCount.
         * @param live The grid the session builds on, swapped.
         * @throws WorldMapError If the index names no city.
         */
        void openCityAt(std::size_t index, const LiveGrid &live);

        /**
         * @brief Put the live grid away with the city it belongs to.
         *
         * Doing this while no city is open changes nothing, which is
         * what makes a stray press on the way-back key a no-op rather
         * than an error.
         *
         * The World is committed before its contents are read, because
         * create() is immediate where add() is staged: a building put up
         * by a click earlier in this very tick, or a walker sent out by
         * the last one, is otherwise not visible yet and would be left
         * behind by the city it belongs to.
         *
         * What is put away is *not* cleared off the live grid here. The
         * next openCityAt() destroys what is standing before it lays its
         * own city down, and nothing draws or steps a grid while the
         * world map is showing -- every system that touches it is gated
         * on AppMode::CityMap.
         *
         * @param live The grid the session builds on, kept with the
         * city.
         */
        void closeCity(const LiveGrid &live);

        /**
         * @brief Get a city's paths, for building on.
         * @param city An index below kCityCount.
         * @return That city's path index.
         * @throws WorldMapError If the index names no city.
         */
        [[nodiscard]] PathIndex &cityPaths(std::size_t city);

        /**
         * @brief Get a city's paths, for drawing.
         * @param city An index below kCityCount.
         * @return That city's path index.
         * @throws WorldMapError If the index names no city.
         */
        [[nodiscard]] const PathIndex &cityPaths(std::size_t city) const;

        /**
         * @brief Get a city's camera, for panning and zooming.
         * @param city An index below kCityCount.
         * @return That city's camera.
         * @throws WorldMapError If the index names no city.
         */
        [[nodiscard]] Camera &cityCamera(std::size_t city);

        /**
         * @brief Get a city's camera, for drawing.
         * @param city An index below kCityCount.
         * @return That city's camera.
         * @throws WorldMapError If the index names no city.
         */
        [[nodiscard]] const Camera &cityCamera(std::size_t city) const;

    private:
        void requireCity(std::size_t city) const;

        WorldMap map;
        bool open = true;
        std::size_t liveCity = 0;
        std::array<PathIndex, kCityCount> paths{};
        std::array<CityGrid, kCityCount> grids{};
        std::array<Camera, kCityCount> cameras{
            Camera{}, Camera{}, Camera{}, Camera{}};
    };

} // namespace antwika::game
