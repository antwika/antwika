#pragma once

#include <array>
#include <cstddef>

#include "antwika/game/Camera.hpp"
#include "antwika/game/PathIndex.hpp"
#include "antwika/game/WorldMap.hpp"

namespace antwika::game
{

    /**
     * @brief Which of the two maps is showing.
     */
    enum class MapView
    {
        World,
        City,
    };

    /**
     * @brief Which map is showing, and what has been built on each
     * city's grid.
     *
     * **Simulation state, not render state**, for exactly the reason
     * Camera is: a click arrives as a pixel, and what that pixel means
     * depends entirely on which map is showing. A view owned by the
     * renderer would leave a replay resolving a recorded click against
     * whichever map it happened to be on -- still deterministically,
     * just deterministically wrong. So this is folded from replayable
     * input like anything else, and the renderer only reads it.
     *
     * The world itself is generated from an integer seed held in
     * WorldMapConfig, so what a replay carries is the seed rather than
     * the map, and the map comes back identical.
     *
     * Each city keeps its own PathIndex and its own Camera, so leaving
     * a city and coming back shows what was built there and shows it
     * from where it was left. They live here rather than in one shared
     * grid because "the grid" is per city: two cities that shared a
     * PathIndex would show each other's roads.
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
         * @brief Get which map is showing.
         * @return World until a city is opened.
         */
        [[nodiscard]] MapView view() const noexcept;

        /**
         * @brief Get which city is open.
         * @return The open city's index.
         * @throws WorldMapError If the world map is showing, since
         * there is then no answer rather than a default one.
         */
        [[nodiscard]] std::size_t openCity() const;

        /**
         * @brief Open a city's map.
         * @param city An index below kCityCount.
         * @throws WorldMapError If the index names no city.
         */
        void openCityAt(std::size_t city);

        /**
         * @brief Go back to the world map, keeping the city's grid.
         *
         * Doing this while the world map is already showing changes
         * nothing, which is what makes a stray press on the way-back
         * key a no-op rather than an error.
         */
        void closeCity();

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
        MapView showing = MapView::World;
        std::size_t city = 0;
        std::array<PathIndex, kCityCount> paths{};
        std::array<Camera, kCityCount> cameras{
            Camera{}, Camera{}, Camera{}, Camera{}};
    };

} // namespace antwika::game
