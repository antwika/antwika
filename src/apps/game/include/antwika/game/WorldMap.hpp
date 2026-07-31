#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

#include <antwika/wfc/Domain.hpp>

#include "antwika/game/Cell.hpp"
#include "antwika/game/Terrain.hpp"

namespace antwika::game
{

    /**
     * @brief How many cities a world map has.
     *
     * Exactly four, and that is a placement rule rather than anything
     * the solver is asked for -- see placeCities().
     */
    inline constexpr std::size_t kCityCount = 4;

    /**
     * @brief What a world map is generated from.
     *
     * A plain value, so the whole world is a pure function of it. The
     * seed is an integer that lives in application state, which is what
     * lets a replay regenerate the identical map instead of having to
     * carry it.
     */
    struct WorldMapConfig
    {
        std::uint32_t width = 24;
        std::uint32_t height = 16;
        std::uint64_t seed = 0;

        /**
         * @brief Compare two configurations.
         * @param other The configuration to compare against.
         * @return True when every field matches.
         */
        [[nodiscard]] bool operator==(
            const WorldMapConfig &other) const = default;
    };

    /**
     * @brief A solved world: its terrain, and the four cities on it.
     *
     * Row-major and index-addressed, in the same flat shape
     * apps/sudoku hands antwika::wfc, because the solver has no notion
     * of a grid: every piece of geometry here is expressed as a
     * constraint over indices, never as something the library knows.
     *
     * A plain comparable value, so "the same seed makes the same world"
     * is one EXPECT_EQ rather than a tile-by-tile walk.
     */
    struct WorldMap
    {
        std::uint32_t width = 0;
        std::uint32_t height = 0;

        /// One terrain per cell, row-major, width * height of them.
        std::vector<Terrain> tiles;

        /// Flat indices into tiles, ascending, all distinct, all land.
        std::array<std::size_t, kCityCount> cities{};

        /**
         * @brief Get the terrain at a coordinate.
         * @param x Column, which must be below width.
         * @param y Row, which must be below height.
         * @return The terrain there.
         * @throws WorldMapError If the coordinate is off the map.
         */
        [[nodiscard]] Terrain at(
            std::uint32_t x, std::uint32_t y) const;

        /**
         * @brief Get where a city sits.
         * @param city An index below kCityCount.
         * @return The city's coordinate on the map.
         * @throws WorldMapError If the index names no city.
         */
        [[nodiscard]] Cell cityCell(std::size_t city) const;

        /**
         * @brief Find the city on a coordinate, if any.
         * @param cell The coordinate to ask about.
         * @return The city's index, or kCityCount for no city there.
         */
        [[nodiscard]] std::size_t cityAt(Cell cell) const;

        /**
         * @brief Compare two maps.
         * @param other The map to compare against.
         * @return True when the size, the tiles and the cities match.
         */
        [[nodiscard]] bool operator==(const WorldMap &other) const = default;
    };

    /**
     * @brief Build the initial antwika::wfc wave for a world.
     *
     * Every cell starts with the whole alphabet except the anchors --
     * a lattice two cells apart, each pinned to one terrain drawn from
     * the seed. The seed reaches the map only through this function:
     * antwika::wfc::Solver takes no seed and is deterministic on its
     * input, so the anchors are the only place randomness can enter.
     *
     * Each anchor is drawn from the range its already-chosen left and
     * upper lattice neighbours leave open, namely within two ladder
     * steps of both. Two pinned cells that far apart on a grid always
     * have a valid path of terrains between them, so the wave this
     * returns is satisfiable however the seed falls.
     *
     * @param config The size and seed to build from.
     * @return One Domain per cell, row-major, width * height of them.
     */
    [[nodiscard]] std::vector<antwika::wfc::Domain> buildWorldWave(
        const WorldMapConfig &config);

    /**
     * @brief Solve a wave into terrain.
     *
     * The geometry lives entirely in the constraints built here: one
     * AdjacencyConstraint per orthogonally neighbouring pair, each
     * carrying the same elevation-ladder CompatibilityTable. The
     * solver is handed no grid, no width and no notion of a neighbour.
     *
     * No step budget is set, so the only two outcomes are a solution
     * and an exhausted search.
     *
     * @param width The map's width in cells.
     * @param height The map's height in cells.
     * @param wave One Domain per cell, row-major.
     * @return The solved terrain, row-major.
     * @throws WorldMapError If the wave is the wrong length, or if no
     * assignment satisfies the ladder.
     */
    [[nodiscard]] std::vector<Terrain> solveTerrain(
        std::uint32_t width,
        std::uint32_t height,
        std::vector<antwika::wfc::Domain> wave);

    /**
     * @brief Choose the four city sites on already-solved terrain.
     *
     * **The solver does not place these.** antwika::wfc is a constraint
     * solver, not a counter, and there is no honest way to ask it for
     * "exactly four of something" without a constraint that encodes a
     * count. So the terrain is solved first and the cities are then
     * placed by a deterministic rule over the answer: the map is cut
     * into quadrants, and each quadrant contributes its best land cell.
     * "Best" is the number of land cells among its four neighbours,
     * then the terrain nearest to plains, then the lowest flat index --
     * three integer comparisons, no floating point, no tie left open.
     *
     * A quadrant with no land at all falls back to the best unclaimed
     * land cell anywhere, so four is a guarantee rather than a hope.
     *
     * @param width The map's width in cells.
     * @param height The map's height in cells.
     * @param tiles The solved terrain, row-major, width * height long.
     * @return The four sites, as flat indices, in ascending order.
     * @throws WorldMapError If tiles is the wrong length, or if fewer
     * than kCityCount of them are land.
     */
    [[nodiscard]] std::array<std::size_t, kCityCount> placeCities(
        std::uint32_t width,
        std::uint32_t height,
        const std::vector<Terrain> &tiles);

    /**
     * @brief Generate a world.
     *
     * The terrain comes out of antwika::wfc: one cell per tile, an
     * AdjacencyConstraint between every orthogonally neighbouring pair
     * carrying the elevation ladder from Terrain.hpp, and a sparse
     * lattice of seeded anchor cells pinned to singleton domains. The
     * solver is itself deterministic and takes no seed, so the seed's
     * whole job is choosing those anchors; the solver then works out
     * every cell between them, which is what makes the result coherent
     * rather than noise.
     *
     * The anchors are placed two cells apart and each is drawn from the
     * range its already-chosen neighbours leave open, which keeps the
     * pinned values within reach of each other. That is what makes the
     * wave satisfiable by construction rather than by retrying: a
     * partial assignment extends to a whole one exactly when no two
     * pinned cells differ by more than the distance between them.
     *
     * @param config The size and seed to generate from.
     * @return The solved world, with its four cities placed.
     * @throws WorldMapError If the map is smaller than 4x4, or if the
     * solver could not satisfy the wave, or if the terrain left too
     * little land for four cities.
     */
    [[nodiscard]] WorldMap generateWorldMap(const WorldMapConfig &config);

} // namespace antwika::game
