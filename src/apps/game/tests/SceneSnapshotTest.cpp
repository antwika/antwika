#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <antwika/ecs/World.hpp>
#include <antwika/log/mocks/MockLogger.hpp>

#include "antwika/game/Building.hpp"
#include "antwika/game/Camera.hpp"
#include "antwika/game/Cell.hpp"
#include "antwika/game/Direction.hpp"
#include "antwika/game/GridExtent.hpp"
#include "antwika/game/Household.hpp"
#include "antwika/game/HousingLevel.hpp"
#include "antwika/game/PathIndex.hpp"
#include "antwika/game/Ruin.hpp"
#include "antwika/game/GameSummary.hpp"
#include "antwika/game/SceneSnapshot.hpp"
#include "antwika/game/Walker.hpp"

using antwika::ecs::World;
using antwika::game::Camera;
using antwika::game::Cell;
using antwika::game::Direction;
using antwika::game::GridExtent;
using antwika::game::kTicksPerStep;
using antwika::game::PathIndex;
using antwika::game::snapshotOf;
using antwika::game::Walker;
using antwika::game::walkerViewsOf;
using antwika::game::WalkerSprite;
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

// The pause is simulation state a picture reads, like the camera.
// A held walker is drawn where its step got to and no further.
// So a scene has to be told, and this is what tells it.
TEST(SceneSnapshotTest, SnapshotOf_CarriesThePauseThrough)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    const PathIndex paths;

    const auto running = snapshotOf(world, paths, Camera(), kExtent);
    const auto held = snapshotOf(world, paths, Camera(), kExtent, true);

    // Defaulted to the state a run begins in.
    EXPECT_FALSE(running.paused);
    EXPECT_TRUE(held.paused);
    EXPECT_NE(running, held);
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
        (std::vector<WalkerSprite>{
            {.at = {.x = 3, .y = 4}, .facing = Direction::South}}));
}

TEST(SceneSnapshotTest, SnapshotOf_TakesWhereAWalkerSteppedOutOf)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    const PathIndex paths;

    const auto entity = world.create();
    world.add<Cell>(entity, Cell{.x = 3, .y = 4});
    world.add<Walker>(
        entity,
        Walker{
            .facing = Direction::East,
            .ticksUntilStep = 1,
            .from = Cell{.x = 2, .y = 4}});
    world.commit();

    const auto snapshot = snapshotOf(world, paths, Camera(), kExtent);

    ASSERT_EQ(snapshot.walkers.size(), 1U);
    EXPECT_EQ(snapshot.walkers[0].from, (Cell{.x = 2, .y = 4}));
}

TEST(SceneSnapshotTest, SnapshotOf_CountsUpHowFarThroughAStepAWalkerIs)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    const PathIndex paths;

    // A walker counts down to its next step.
    // So the tick it stepped on is furthest from stepping again.
    // And is nought ticks into the step it is drawn part way through.
    const auto stepped = world.create();
    world.add<Cell>(stepped, Cell{.x = 1, .y = 0});
    world.add<Walker>(
        stepped,
        Walker{
            .ticksUntilStep = kTicksPerStep - 1,
            .from = Cell{.x = 0, .y = 0}});

    const auto waited = world.create();
    world.add<Cell>(waited, Cell{.x = 5, .y = 0});
    world.add<Walker>(
        waited,
        Walker{.ticksUntilStep = 0, .from = Cell{.x = 4, .y = 0}});

    world.commit();

    const auto snapshot = snapshotOf(world, paths, Camera(), kExtent);

    ASSERT_EQ(snapshot.walkers.size(), 2U);
    EXPECT_EQ(snapshot.walkers[0].ticksIntoStep, 0U);
    EXPECT_EQ(
        snapshot.walkers[1].ticksIntoStep, kTicksPerStep - 1);
}

TEST(SceneSnapshotTest, SnapshotOf_LeavesAWalkerThatNeverSteppedAtNothing)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    const PathIndex paths;

    // Due to step, and never having stepped: nowhere through one.
    // Rather than as far through one as a walker that has moved.
    const auto entity = world.create();
    world.add<Cell>(entity, Cell{.x = 3, .y = 4});
    world.add<Walker>(entity, Walker{.ticksUntilStep = 0});
    world.commit();

    const auto snapshot = snapshotOf(world, paths, Camera(), kExtent);

    ASSERT_EQ(snapshot.walkers.size(), 1U);
    EXPECT_FALSE(snapshot.walkers[0].from.has_value());
    EXPECT_EQ(snapshot.walkers[0].ticksIntoStep, 0U);
}

TEST(SceneSnapshotTest, WalkerViewsOf_ReportsWhereEachWalkerIs)
{
    NiceMock<MockLogger> logger;
    World world(logger);

    // The state answer, so it says nothing about being part way.
    // Even when the walker it describes is.
    const auto entity = world.create();
    world.add<Cell>(entity, Cell{.x = 3, .y = 4});
    world.add<Walker>(
        entity,
        Walker{
            .facing = Direction::South,
            .ticksUntilStep = 1,
            .from = Cell{.x = 3, .y = 3}});
    world.commit();

    EXPECT_EQ(
        walkerViewsOf(world),
        (std::vector<WalkerView>{
            {.at = {.x = 3, .y = 4}, .facing = Direction::South}}));
}

TEST(SceneSnapshotTest, WalkerViewsOf_ReportsNothingWithNoWalkers)
{
    NiceMock<MockLogger> logger;
    World world(logger);

    EXPECT_TRUE(walkerViewsOf(world).empty());
}

TEST(SceneSnapshotTest, WalkerSpriteEqualityComparesEveryField)
{
    constexpr WalkerSprite sprite{
        .at = {.x = 1, .y = 2},
        .facing = Direction::North,
        .from = Cell{.x = 1, .y = 3},
        .ticksIntoStep = 1};

    EXPECT_EQ(sprite, sprite);

    auto elsewhere = sprite;
    elsewhere.at = Cell{.x = 9, .y = 9};
    EXPECT_NE(sprite, elsewhere);

    auto turned = sprite;
    turned.facing = Direction::South;
    EXPECT_NE(sprite, turned);

    auto unmoved = sprite;
    unmoved.from.reset();
    EXPECT_NE(sprite, unmoved);

    auto later = sprite;
    later.ticksIntoStep = 0;
    EXPECT_NE(sprite, later);

    auto other = sprite;
    other.kind = antwika::game::WalkerKind::MarketSeller;
    EXPECT_NE(sprite, other);

    auto spent = sprite;
    spent.carried = sprite.carried + 1;
    EXPECT_NE(sprite, spent);
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

// GameSummary's defaulted comparison short-circuits.
// So each field needs a pair that differs in it alone.
TEST(SceneSnapshotTest, GameSummaryEqualityComparesEveryField)
{
    using antwika::game::Camera;
    using antwika::game::GameState;
    using antwika::game::GameSummary;

    const GameSummary base{
        .state = {.ticksProcessed = 1, .score = 2},
        .paths = {Cell{.x = 1, .y = 1}},
        .walkers = {WalkerView{.at = {.x = 2, .y = 2}}},
        .buildings = {},
        .ruins = {},
        .camera = Camera(),
        .ratings = {},
        .console = {},
        .bindings = {}};

    EXPECT_EQ(base, base);

    auto scored = base;
    scored.state.score = 99;
    EXPECT_NE(base, scored);

    auto paved = base;
    paved.paths.push_back(Cell{.x = 5, .y = 5});
    EXPECT_NE(base, paved);

    auto peopled = base;
    peopled.walkers.clear();
    EXPECT_NE(base, peopled);

    auto moved = base;
    moved.camera.panBy(1, 0);
    EXPECT_NE(base, moved);

    auto rated = base;
    rated.ratings.population = 7;
    EXPECT_NE(base, rated);
}

// With one-cell buildings two could never overlap.
// So placement order was as good as any.
// A block drawn before what is behind it is the wrong picture.
TEST(SceneSnapshotTest, SnapshotOf_OrdersTheBuildingsBackToFront)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    const PathIndex paths;

    // Added front to back, so placement order is the wrong order.
    for (const auto at : {
             antwika::game::Cell{.x = 3, .y = 3},
             antwika::game::Cell{.x = 0, .y = 1},
             antwika::game::Cell{.x = 1, .y = 0}})
    {
        const auto entity = world.create();
        world.add<Cell>(entity, at);
        world.add<antwika::game::Building>(
            entity, antwika::game::Building{});
    }
    world.commit();

    const auto snapshot = snapshotOf(world, paths, Camera(), kExtent);

    ASSERT_EQ(snapshot.buildings.size(), 3U);

    // Depth is x + y, and the tie between (0,1) and (1,0) breaks on x.
    EXPECT_EQ(snapshot.buildings[0].at, (Cell{.x = 0, .y = 1}));
    EXPECT_EQ(snapshot.buildings[1].at, (Cell{.x = 1, .y = 0}));
    EXPECT_EQ(snapshot.buildings[2].at, (Cell{.x = 3, .y = 3}));
}

TEST(SceneSnapshotTest, SnapshotOf_TakesWhatEachWalkerIsCarrying)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    const PathIndex paths;

    const auto entity = world.create();
    world.add<Cell>(entity, Cell{.x = 1, .y = 1});
    world.add<Walker>(
        entity,
        Walker{
            .kind = antwika::game::WalkerKind::WaterCarrier, .carried = 42});
    world.commit();

    const auto snapshot = snapshotOf(world, paths, Camera(), kExtent);

    ASSERT_EQ(snapshot.walkers.size(), 1U);
    EXPECT_EQ(
        snapshot.walkers[0].kind,
        antwika::game::WalkerKind::WaterCarrier);
    EXPECT_EQ(snapshot.walkers[0].carried, 42);
}

// The picture needs the stock; the state a summary compares must not.
// So the sprite carries it and BuildingView does not -- see WalkerView.
TEST(SceneSnapshotTest, SnapshotOf_TakesWhatEachBuildingIsHolding)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    const PathIndex paths;

    const auto entity = world.create();
    world.add<Cell>(entity, Cell{.x = 2, .y = 2});
    world.add<antwika::game::Building>(
        entity,
        antwika::game::Building{
            .kind = antwika::game::BuildingKind::House,
            .stock = {17, 71}});
    world.commit();

    const auto snapshot = snapshotOf(world, paths, Camera(), kExtent);

    ASSERT_EQ(snapshot.buildings.size(), 1U);
    EXPECT_EQ(snapshot.buildings[0].at, (Cell{.x = 2, .y = 2}));
    EXPECT_EQ(
        snapshot.buildings[0].kind, antwika::game::BuildingKind::House);
    EXPECT_EQ(snapshot.buildings[0].stock[0], 17);
    EXPECT_EQ(snapshot.buildings[0].stock[1], 71);
}

TEST(SceneSnapshotTest, BuildingViewsOf_ReportsEachBuildingAsState)
{
    NiceMock<MockLogger> logger;
    World world(logger);

    // Put up front to back, which is the order a session had them in.
    // A summary reports a session rather than a screen, so it keeps it.
    for (const auto at : {
             Cell{.x = 3, .y = 3},
             Cell{.x = 0, .y = 1}})
    {
        const auto entity = world.create();
        world.add<Cell>(entity, at);
        world.add<antwika::game::Building>(
            entity,
            antwika::game::Building{
                .kind = antwika::game::BuildingKind::House,
                .stock = {5, 6}});
    }
    world.commit();

    EXPECT_EQ(
        antwika::game::buildingViewsOf(world),
        (std::vector<antwika::game::BuildingView>{
            {.at = {.x = 3, .y = 3},
             .kind = antwika::game::BuildingKind::House},
            {.at = {.x = 0, .y = 1},
             .kind = antwika::game::BuildingKind::House}}));
}

TEST(SceneSnapshotTest, BuildingViewsOf_ReportsNothingWithNoBuildings)
{
    NiceMock<MockLogger> logger;
    World world(logger);

    EXPECT_TRUE(antwika::game::buildingViewsOf(world).empty());
}

// The level is state a summary compares rather than a picture.
// So it goes onto the view as well as onto the sprite.
TEST(SceneSnapshotTest, TakesEachHousesLevelOntoBothViewAndSprite)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    const PathIndex paths;

    const auto grown = world.create();
    world.add<Cell>(grown, Cell{.x = 1, .y = 1});
    world.add<antwika::game::Building>(
        grown,
        antwika::game::Building{
            .kind = antwika::game::BuildingKind::House});
    antwika::game::setHousehold(
        world,
        grown,
        antwika::game::Household{
            .level = antwika::game::HousingLevel::Hovel});

    const auto fresh = world.create();
    world.add<Cell>(fresh, Cell{.x = 5, .y = 5});
    world.add<antwika::game::Building>(
        fresh,
        antwika::game::Building{
            .kind = antwika::game::BuildingKind::House});
    world.commit();

    const auto views = antwika::game::buildingViewsOf(world);

    ASSERT_EQ(views.size(), 2U);
    EXPECT_EQ(views[0].level, antwika::game::HousingLevel::Hovel);
    EXPECT_EQ(views[1].level, antwika::game::HousingLevel::Tent);

    const auto snapshot =
        snapshotOf(world, paths, Camera(), kExtent);

    ASSERT_EQ(snapshot.buildings.size(), 2U);
    EXPECT_EQ(
        snapshot.buildings[0].level, antwika::game::HousingLevel::Hovel);
    EXPECT_EQ(
        snapshot.buildings[1].level, antwika::game::HousingLevel::Tent);
}

// Occupancy goes onto the view as well, and that is the point of it.
// A sum over the city hides two houses swapping one occupant.
// Compared per house, a run and its replay cannot hide that.
TEST(SceneSnapshotTest, TakesEachHousesPeopleOntoBothViewAndSprite)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    const PathIndex paths;

    const auto lived = world.create();
    world.add<Cell>(lived, Cell{.x = 1, .y = 1});
    world.add<antwika::game::Building>(
        lived,
        antwika::game::Building{
            .kind = antwika::game::BuildingKind::House});
    antwika::game::setHousehold(
        world, lived, antwika::game::Household{.population = 4});

    const auto empty = world.create();
    world.add<Cell>(empty, Cell{.x = 5, .y = 5});
    world.add<antwika::game::Building>(
        empty,
        antwika::game::Building{
            .kind = antwika::game::BuildingKind::House});
    world.commit();

    const auto views = antwika::game::buildingViewsOf(world);

    ASSERT_EQ(views.size(), 2U);
    EXPECT_EQ(views[0].population, 4);
    EXPECT_EQ(views[1].population, 0);

    const auto snapshot = snapshotOf(world, paths, Camera(), kExtent);

    ASSERT_EQ(snapshot.buildings.size(), 2U);
    EXPECT_EQ(snapshot.buildings[0].population, 4);
    EXPECT_EQ(snapshot.buildings[1].population, 0);
}

TEST(SceneSnapshotTest, SnapshotOf_TakesEveryRuinInPaintersOrder)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    const PathIndex paths;

    // The deeper one is created first, so the sort has work to do.
    const auto deep = world.create();
    world.add<Cell>(deep, Cell{.x = 3, .y = 3});
    world.add<antwika::game::Ruin>(
        deep,
        antwika::game::Ruin{
            .kind = antwika::game::BuildingKind::Farm,
            .state = antwika::game::RuinState::Debris,
            .ticksUntilOut = 0});

    const auto shallow = world.create();
    world.add<Cell>(shallow, Cell{.x = 1, .y = 1});
    world.add<antwika::game::Ruin>(
        shallow,
        antwika::game::Ruin{
            .kind = antwika::game::BuildingKind::House});

    // On the deep one's own diagonal, so the tie-break has work too.
    const auto tied = world.create();
    world.add<Cell>(tied, Cell{.x = 2, .y = 4});
    world.add<antwika::game::Ruin>(
        tied,
        antwika::game::Ruin{
            .kind = antwika::game::BuildingKind::House});
    world.commit();

    const auto snapshot =
        snapshotOf(world, paths, Camera(), kExtent);

    ASSERT_EQ(snapshot.ruins.size(), 3U);
    EXPECT_EQ(snapshot.ruins[0].at, (Cell{.x = 1, .y = 1}));
    EXPECT_EQ(
        snapshot.ruins[0].kind, antwika::game::BuildingKind::House);
    EXPECT_EQ(
        snapshot.ruins[0].state, antwika::game::RuinState::Burning);
    EXPECT_EQ(snapshot.ruins[1].at, (Cell{.x = 2, .y = 4}));
    EXPECT_EQ(snapshot.ruins[2].at, (Cell{.x = 3, .y = 3}));
    EXPECT_EQ(
        snapshot.ruins[2].state, antwika::game::RuinState::Debris);
}

TEST(SceneSnapshotTest, RuinViewsOf_ReportsEachRuinAsState)
{
    NiceMock<MockLogger> logger;
    World world(logger);

    const auto fire = world.create();
    world.add<Cell>(fire, Cell{.x = 2, .y = 5});
    world.add<antwika::game::Ruin>(
        fire,
        antwika::game::Ruin{
            .kind = antwika::game::BuildingKind::Market});
    world.commit();

    const auto views = antwika::game::ruinViewsOf(world);

    ASSERT_EQ(views.size(), 1U);
    EXPECT_EQ(views[0].at, (Cell{.x = 2, .y = 5}));
    EXPECT_EQ(views[0].kind, antwika::game::BuildingKind::Market);
    EXPECT_EQ(views[0].state, antwika::game::RuinState::Burning);
}

TEST(SceneSnapshotTest, RuinViewsOf_ReportsNothingWithNoRuins)
{
    NiceMock<MockLogger> logger;
    World world(logger);

    EXPECT_TRUE(antwika::game::ruinViewsOf(world).empty());
}
