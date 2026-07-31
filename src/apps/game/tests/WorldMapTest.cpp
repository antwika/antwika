#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <set>
#include <vector>

#include <gtest/gtest.h>

#include <antwika/wfc/Domain.hpp>

#include "antwika/game/WorldMap.hpp"
#include "antwika/game/WorldMapError.hpp"

namespace
{

    using antwika::game::Cell;
    using antwika::game::generateWorldMap;
    using antwika::game::isLand;
    using antwika::game::symbolOf;
    using antwika::game::kCityCount;
    using antwika::game::kTerrainCount;
    using antwika::game::placeCities;
    using antwika::game::solveTerrain;
    using antwika::game::Terrain;
    using antwika::game::terrainOf;
    using antwika::game::WorldMap;
    using antwika::game::WorldMapConfig;
    using antwika::game::WorldMapError;
    using antwika::wfc::Domain;

    std::vector<Terrain> tilesOf(
        std::uint32_t width,
        std::uint32_t height,
        Terrain fill,
        const std::vector<std::size_t> &water)
    {
        std::vector<Terrain> tiles(
            static_cast<std::size_t>(width) * height, fill);
        for (const std::size_t index : water)
        {
            tiles[index] = Terrain::Water;
        }
        return tiles;
    }

    TEST(TerrainTest, WaterIsTheOnlyThingThatIsNotLand)
    {
        EXPECT_FALSE(isLand(Terrain::Water));
        EXPECT_TRUE(isLand(Terrain::Plains));
        EXPECT_TRUE(isLand(Terrain::Mountain));
    }

    TEST(TerrainTest, SymbolsRoundTripAndClampAboveTheAlphabet)
    {
        for (std::size_t symbol = 0; symbol < kTerrainCount; ++symbol)
        {
            EXPECT_EQ(symbolOf(terrainOf(symbol)), symbol);
        }
        EXPECT_EQ(terrainOf(kTerrainCount), Terrain::Mountain);
    }

    TEST(WorldMapTest, TheSameSeedMakesTheSameWorld)
    {
        const WorldMapConfig config{16, 12, 4242};
        EXPECT_EQ(generateWorldMap(config), generateWorldMap(config));
    }

    TEST(WorldMapTest, ADifferentSeedMakesADifferentWorld)
    {
        const WorldMap first = generateWorldMap({16, 12, 1});
        const WorldMap second = generateWorldMap({16, 12, 2});
        EXPECT_NE(first.tiles, second.tiles);
    }

    TEST(WorldMapTest, EveryNeighbourIsOneStepAlongTheLadder)
    {
        for (std::uint64_t seed = 0; seed < 6; ++seed)
        {
            const WorldMap map = generateWorldMap({14, 10, seed});
            for (std::uint32_t y = 0; y < map.height; ++y)
            {
                for (std::uint32_t x = 0; x < map.width; ++x)
                {
                    const auto here =
                        static_cast<int>(symbolOf(map.at(x, y)));
                    if (x + 1 < map.width)
                    {
                        const auto right =
                            static_cast<int>(symbolOf(map.at(x + 1, y)));
                        EXPECT_LE(std::abs(here - right), 1);
                    }
                    if (y + 1 < map.height)
                    {
                        const auto below =
                            static_cast<int>(symbolOf(map.at(x, y + 1)));
                        EXPECT_LE(std::abs(here - below), 1);
                    }
                }
            }
        }
    }

    TEST(WorldMapTest, HasExactlyFourDistinctCitiesOnLand)
    {
        for (std::uint64_t seed = 0; seed < 6; ++seed)
        {
            const WorldMap map = generateWorldMap({14, 10, seed});
            const std::set<std::size_t> distinct(
                map.cities.begin(), map.cities.end());
            EXPECT_EQ(distinct.size(), kCityCount);
            EXPECT_TRUE(std::ranges::is_sorted(map.cities));
            for (const std::size_t index : map.cities)
            {
                ASSERT_LT(index, map.tiles.size());
                EXPECT_TRUE(isLand(map.tiles[index]));
            }
        }
    }

    TEST(WorldMapTest, CityCellsAndLookupAgree)
    {
        const WorldMap map = generateWorldMap({14, 10, 7});
        for (std::size_t city = 0; city < kCityCount; ++city)
        {
            const Cell cell = map.cityCell(city);
            EXPECT_EQ(map.cityAt(cell), city);
        }
    }

    TEST(WorldMapTest, ACellWithNoCityReportsNone)
    {
        WorldMap map;
        map.width = 4;
        map.height = 4;
        map.tiles = tilesOf(4, 4, Terrain::Plains, {});
        map.cities = {0, 1, 2, 3};
        EXPECT_EQ(map.cityAt(Cell{0, 3}), kCityCount);
    }

    TEST(WorldMapTest, AskingOffTheMapThrows)
    {
        const WorldMap map = generateWorldMap({8, 8, 3});
        EXPECT_THROW((void)map.at(8, 0), WorldMapError);
        EXPECT_THROW((void)map.at(0, 8), WorldMapError);
        EXPECT_THROW((void)map.cityCell(kCityCount), WorldMapError);
    }

    TEST(WorldMapTest, ATinyMapIsRefused)
    {
        EXPECT_THROW(
            (void)generateWorldMap({3, 8, 0}), WorldMapError);
        EXPECT_THROW(
            (void)generateWorldMap({8, 3, 0}), WorldMapError);
    }

    TEST(WorldMapTest, AWaveOfTheWrongLengthIsRefused)
    {
        std::vector<Domain> wave(3, Domain(kTerrainCount));
        EXPECT_THROW(
            (void)solveTerrain(4, 4, std::move(wave)), WorldMapError);
    }

    TEST(WorldMapTest, AWaveThatCannotBeSatisfiedIsRefused)
    {
        std::vector<Domain> wave(16, Domain(kTerrainCount));
        wave[0] = Domain::singleton(0, kTerrainCount);
        wave[1] = Domain::singleton(4, kTerrainCount);
        EXPECT_THROW(
            (void)solveTerrain(4, 4, std::move(wave)), WorldMapError);
    }

    TEST(WorldMapTest, TilesOfTheWrongLengthAreRefused)
    {
        const std::vector<Terrain> tiles(3, Terrain::Plains);
        EXPECT_THROW((void)placeCities(4, 4, tiles), WorldMapError);
    }

    TEST(WorldMapTest, TooLittleLandIsRefused)
    {
        const std::vector<Terrain> tiles =
            tilesOf(4, 4, Terrain::Water, {});
        EXPECT_THROW((void)placeCities(4, 4, tiles), WorldMapError);
    }

    TEST(WorldMapTest, AQuadrantWithNoLandStillGetsACity)
    {
        // The whole top-left quadrant is water.
        // Its city has to come from the fallback scan.
        std::vector<Terrain> tiles =
            tilesOf(4, 4, Terrain::Plains, {0, 1, 4, 5});
        const std::array<std::size_t, kCityCount> cities =
            placeCities(4, 4, tiles);
        const std::set<std::size_t> distinct(
            cities.begin(), cities.end());
        EXPECT_EQ(distinct.size(), kCityCount);
        for (const std::size_t index : cities)
        {
            EXPECT_TRUE(isLand(tiles[index]));
        }
    }

    TEST(WorldMapTest, PlainsBeatsMountainWhenNeighboursTie)
    {
        // All land but for cell 5.
        // Its quadrant's three other cells then tie on two.
        // Nearness to plains is what decides between them.
        std::vector<Terrain> tiles(16, Terrain::Mountain);
        tiles[5] = Terrain::Water;
        tiles[4] = Terrain::Plains;
        const std::array<std::size_t, kCityCount> cities =
            placeCities(4, 4, tiles);
        EXPECT_NE(std::ranges::find(cities, 4U), cities.end());
        EXPECT_EQ(std::ranges::find(cities, 0U), cities.end());
        EXPECT_EQ(std::ranges::find(cities, 1U), cities.end());
    }

    TEST(WorldMapTest, MoreLandNeighboursWins)
    {
        // Row 1 is land and row 0 is water.
        // A land cell with land on each side beats an end one.
        std::vector<Terrain> tiles(16, Terrain::Water);
        tiles[4] = Terrain::Plains;
        tiles[5] = Terrain::Plains;
        tiles[6] = Terrain::Plains;
        tiles[7] = Terrain::Plains;
        const std::array<std::size_t, kCityCount> cities =
            placeCities(4, 4, tiles);
        const std::array<std::size_t, kCityCount> expected{4, 5, 6, 7};
        EXPECT_EQ(cities, expected);
    }

} // namespace
