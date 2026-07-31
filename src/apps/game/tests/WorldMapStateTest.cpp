#include <gtest/gtest.h>

#include "antwika/game/Cell.hpp"
#include "antwika/game/WorldMap.hpp"
#include "antwika/game/WorldMapError.hpp"
#include "antwika/game/WorldMapState.hpp"

namespace
{

    using antwika::game::Cell;
    using antwika::game::generateWorldMap;
    using antwika::game::kCityCount;
    using antwika::game::MapView;
    using antwika::game::WorldMapError;
    using antwika::game::WorldMapState;

    WorldMapState freshState()
    {
        return WorldMapState(generateWorldMap({12, 10, 99}));
    }

    TEST(WorldMapStateTest, StartsOnTheWorldMapWithNoCityOpen)
    {
        const WorldMapState state = freshState();
        EXPECT_EQ(state.view(), MapView::World);
        EXPECT_THROW((void)state.openCity(), WorldMapError);
        EXPECT_EQ(state.world().width, 12U);
    }

    TEST(WorldMapStateTest, OpeningACityShowsIt)
    {
        WorldMapState state = freshState();
        state.openCityAt(2);
        EXPECT_EQ(state.view(), MapView::City);
        EXPECT_EQ(state.openCity(), 2U);
    }

    TEST(WorldMapStateTest, ClosingGoesBackAndIsAlwaysSafe)
    {
        WorldMapState state = freshState();
        state.closeCity();
        EXPECT_EQ(state.view(), MapView::World);

        state.openCityAt(1);
        state.closeCity();
        EXPECT_EQ(state.view(), MapView::World);
    }

    TEST(WorldMapStateTest, EachCityKeepsItsOwnGrid)
    {
        WorldMapState state = freshState();
        state.cityPaths(0).insert(Cell{3, 4});
        state.cityCamera(0).zoomIn();

        EXPECT_TRUE(state.cityPaths(0).has(Cell{3, 4}));
        EXPECT_FALSE(state.cityPaths(1).has(Cell{3, 4}));

        const WorldMapState &readOnly = state;
        EXPECT_EQ(readOnly.cityPaths(0).size(), 1U);
        EXPECT_NE(
            readOnly.cityCamera(0).zoomLevel(),
            readOnly.cityCamera(1).zoomLevel());
    }

    TEST(WorldMapStateTest, NoSuchCityIsRefusedEverywhere)
    {
        WorldMapState state = freshState();
        const WorldMapState &readOnly = state;

        EXPECT_THROW(state.openCityAt(kCityCount), WorldMapError);
        EXPECT_THROW((void)state.cityPaths(kCityCount), WorldMapError);
        EXPECT_THROW((void)state.cityCamera(kCityCount), WorldMapError);
        EXPECT_THROW((void)readOnly.cityPaths(kCityCount), WorldMapError);
        EXPECT_THROW((void)readOnly.cityCamera(kCityCount), WorldMapError);
    }

} // namespace
