#include <gtest/gtest.h>

#include "antwika/game/Camera.hpp"
#include "antwika/game/Cell.hpp"
#include "antwika/game/PathIndex.hpp"
#include "antwika/game/WorldMap.hpp"
#include "antwika/game/WorldMapError.hpp"
#include "antwika/game/WorldMapState.hpp"

namespace
{

    using antwika::game::Camera;
    using antwika::game::Cell;
    using antwika::game::generateWorldMap;
    using antwika::game::kCityCount;
    using antwika::game::PathIndex;
    using antwika::game::WorldMapError;
    using antwika::game::WorldMapState;

    WorldMapState freshState()
    {
        return WorldMapState(generateWorldMap({12, 10, 99}));
    }

    TEST(WorldMapStateTest, StartsWithItsFirstCityOpen)
    {
        const WorldMapState state = freshState();
        EXPECT_TRUE(state.cityOpen());
        EXPECT_EQ(state.city(), 0U);
        EXPECT_EQ(state.world().width, 12U);
    }

    TEST(WorldMapStateTest, OpeningACitySwapsItsGridIn)
    {
        WorldMapState state = freshState();
        state.cityPaths(2).insert(Cell{7, 8});

        PathIndex live;
        Camera camera;
        state.openCityAt(2, live, camera);

        EXPECT_TRUE(state.cityOpen());
        EXPECT_EQ(state.city(), 2U);
        EXPECT_TRUE(live.has(Cell{7, 8}));
    }

    TEST(WorldMapStateTest, ClosingKeepsTheGridWithTheCityItBelongsTo)
    {
        WorldMapState state = freshState();
        PathIndex live;
        Camera camera;

        state.openCityAt(1, live, camera);
        live.insert(Cell{3, 4});
        camera.zoomIn();
        state.closeCity(live, camera);

        EXPECT_FALSE(state.cityOpen());
        EXPECT_TRUE(state.cityPaths(1).has(Cell{3, 4}));
        EXPECT_EQ(state.cityCamera(1), camera);

        // A second close has nothing left to put away.
        // So a stray press on the way-back key is a no-op.
        PathIndex other;
        state.closeCity(other, camera);
        EXPECT_TRUE(state.cityPaths(1).has(Cell{3, 4}));
    }

    TEST(WorldMapStateTest, LeavingACityAndComingBackShowsWhatWasBuilt)
    {
        WorldMapState state = freshState();
        PathIndex live;
        Camera camera;

        state.openCityAt(0, live, camera);
        live.insert(Cell{1, 1});

        state.openCityAt(3, live, camera);
        EXPECT_FALSE(live.has(Cell{1, 1}));

        live.insert(Cell{9, 9});

        state.openCityAt(0, live, camera);
        EXPECT_TRUE(live.has(Cell{1, 1}));
        EXPECT_FALSE(live.has(Cell{9, 9}));
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
        PathIndex live;
        Camera camera;

        EXPECT_THROW(
            state.openCityAt(kCityCount, live, camera), WorldMapError);
        EXPECT_THROW((void)state.cityPaths(kCityCount), WorldMapError);
        EXPECT_THROW((void)state.cityCamera(kCityCount), WorldMapError);
        EXPECT_THROW((void)readOnly.cityPaths(kCityCount), WorldMapError);
        EXPECT_THROW((void)readOnly.cityCamera(kCityCount), WorldMapError);
    }

} // namespace
