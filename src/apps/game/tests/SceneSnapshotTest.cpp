#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <antwika/ecs/World.hpp>
#include <antwika/log/mocks/MockLogger.hpp>

#include "antwika/game/Camera.hpp"
#include "antwika/game/Cell.hpp"
#include "antwika/game/Direction.hpp"
#include "antwika/game/GridExtent.hpp"
#include "antwika/game/PathIndex.hpp"
#include "antwika/game/SceneSnapshot.hpp"
#include "antwika/game/Walker.hpp"

using antwika::ecs::World;
using antwika::game::Camera;
using antwika::game::Cell;
using antwika::game::Direction;
using antwika::game::GridExtent;
using antwika::game::PathIndex;
using antwika::game::snapshotOf;
using antwika::game::Walker;
using antwika::game::WalkerView;
using antwika::log::mocks::MockLogger;
using ::testing::NiceMock;

namespace
{
    constexpr GridExtent kExtent{.width = 8, .height = 8};
} // namespace

TEST(SceneSnapshotTest, SnapshotOf_CarriesTheCameraAndExtentThrough)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    const PathIndex paths;
    const Camera camera(antwika::gfx::Point{.x = 9, .y = 8}, 1);

    const auto snapshot = snapshotOf(world, paths, camera, kExtent);

    EXPECT_EQ(snapshot.camera, camera);
    EXPECT_EQ(snapshot.extent, kExtent);
}

TEST(SceneSnapshotTest, SnapshotOf_IsEmptyForAnEmptyWorld)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    const PathIndex paths;

    const auto snapshot = snapshotOf(world, paths, Camera(), kExtent);

    EXPECT_TRUE(snapshot.paths.empty());
    EXPECT_TRUE(snapshot.walkers.empty());
}

TEST(SceneSnapshotTest, SnapshotOf_TakesThePathsInAscendingOrder)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    PathIndex paths;
    paths.insert(Cell{.x = 2, .y = 0});
    paths.insert(Cell{.x = 0, .y = 5});
    paths.insert(Cell{.x = 1, .y = 1});

    const auto snapshot = snapshotOf(world, paths, Camera(), kExtent);

    EXPECT_EQ(
        snapshot.paths,
        (std::vector<Cell>{
            {.x = 0, .y = 5}, {.x = 1, .y = 1}, {.x = 2, .y = 0}}));
}

TEST(SceneSnapshotTest, SnapshotOf_TakesEachWalkersCellAndFacing)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    const PathIndex paths;

    const auto entity = world.create();
    world.add<Cell>(entity, Cell{.x = 3, .y = 4});
    world.add<Walker>(entity, Walker{.facing = Direction::South});
    world.commit();

    const auto snapshot = snapshotOf(world, paths, Camera(), kExtent);

    EXPECT_EQ(
        snapshot.walkers,
        (std::vector<WalkerView>{
            {.at = {.x = 3, .y = 4}, .facing = Direction::South}}));
}

TEST(SceneSnapshotTest, SnapshotOf_TakesEveryWalker)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    const PathIndex paths;

    for (std::int32_t i = 0; i < 3; ++i)
    {
        const auto entity = world.create();
        world.add<Cell>(entity, Cell{.x = i, .y = 0});
        world.add<Walker>(entity, Walker{});
    }
    world.commit();

    const auto snapshot = snapshotOf(world, paths, Camera(), kExtent);

    EXPECT_EQ(snapshot.walkers.size(), 3U);
}

TEST(SceneSnapshotTest, SnapshotOf_IgnoresAnEntityThatIsNotAWalker)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    const PathIndex paths;

    const auto tile = world.create();
    world.add<Cell>(tile, Cell{});
    world.commit();

    const auto snapshot = snapshotOf(world, paths, Camera(), kExtent);

    EXPECT_TRUE(snapshot.walkers.empty());
}

TEST(SceneSnapshotTest, WalkerViewEqualityComparesBothFields)
{
    constexpr WalkerView view{
        .at = {.x = 1, .y = 2}, .facing = Direction::North};

    EXPECT_EQ(view, (WalkerView{.at = {.x = 1, .y = 2},
                                .facing = Direction::North}));
    EXPECT_NE(view, (WalkerView{.at = {.x = 1, .y = 3},
                                .facing = Direction::North}));
    EXPECT_NE(view, (WalkerView{.at = {.x = 1, .y = 2},
                                .facing = Direction::West}));
}

TEST(SceneSnapshotTest, SnapshotEqualityComparesEveryField)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    PathIndex paths;
    paths.insert(Cell{});

    const auto one = snapshotOf(world, paths, Camera(), kExtent);
    const auto same = snapshotOf(world, paths, Camera(), kExtent);
    const auto elsewhere = snapshotOf(
        world, paths, Camera(antwika::gfx::Point{.x = 1, .y = 0}), kExtent);
    const PathIndex empty;
    const auto bare = snapshotOf(world, empty, Camera(), kExtent);

    EXPECT_EQ(one, same);
    EXPECT_NE(one, elsewhere);
    EXPECT_NE(one, bare);
}

// The defaulted comparison short-circuits.
// So each field needs a pair that differs in it alone.
TEST(SceneSnapshotTest, SnapshotEqualityComparesTheExtentAndTheWalkers)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    const PathIndex paths;

    const auto base = snapshotOf(world, paths, Camera(), kExtent);
    const auto wider = snapshotOf(
        world, paths, Camera(), GridExtent{.width = 9, .height = 8});

    EXPECT_NE(base, wider);

    const auto entity = world.create();
    world.add<Cell>(entity, Cell{});
    world.add<Walker>(entity, Walker{});
    world.commit();

    const auto peopled = snapshotOf(world, paths, Camera(), kExtent);

    EXPECT_NE(base, peopled);
    EXPECT_EQ(peopled, snapshotOf(world, paths, Camera(), kExtent));
}
