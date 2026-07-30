#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <vector>

#include <antwika/ecs/World.hpp>
#include <antwika/log/mocks/MockLogger.hpp>

#include "antwika/game/Building.hpp"
#include "antwika/game/Cell.hpp"
#include "antwika/game/Direction.hpp"
#include "antwika/game/PathIndex.hpp"
#include "antwika/game/Walker.hpp"
#include "antwika/game/WalkerSystem.hpp"

using antwika::ecs::Entity;
using antwika::ecs::World;
using antwika::game::Building;
using antwika::game::BuildingKind;
using antwika::game::Cell;
using antwika::game::Direction;
using antwika::game::kMaxRisk;
using antwika::game::kMaxWalkDistance;
using antwika::game::kRiskRelief;
using antwika::game::newlyBuilt;
using antwika::game::newlySpawned;
using antwika::game::PathIndex;
using antwika::game::Resource;
using antwika::game::serve;
using antwika::game::Walker;
using antwika::game::WalkerKind;
using antwika::game::WalkerSystem;
using antwika::log::mocks::MockLogger;

namespace
{
    class WalkerSystemTest : public ::testing::Test
    {
    protected:
        [[nodiscard]] Entity addWalker(Cell at, Direction facing)
        {
            const auto entity = world.create();
            world.add<Cell>(entity, at);
            world.add<Walker>(entity, Walker{.facing = facing});
            world.commit();
            return entity;
        }

        [[nodiscard]] Entity addCarrier(
            Cell at, Direction facing, WalkerKind kind)
        {
            const auto entity = world.create();
            world.add<Cell>(entity, at);
            world.add<Walker>(entity, newlySpawned(kind, facing, at));
            world.commit();
            return entity;
        }

        // A walker that set out from somewhere other than where it is.
        [[nodiscard]] Entity addReturner(
            Cell at, Direction facing, Cell origin)
        {
            const auto entity = world.create();
            world.add<Cell>(entity, at);
            world.add<Walker>(
                entity,
                Walker{
                    .facing = facing,
                    .stepsTaken = kMaxWalkDistance,
                    .origin = origin});
            world.commit();
            return entity;
        }

        [[nodiscard]] Entity addBuilding(Cell at, const Building &building)
        {
            const auto entity = world.create();
            world.add<Cell>(entity, at);
            world.add<Building>(entity, building);
            world.commit();
            return entity;
        }

        void layPath(const std::vector<Cell> &cells)
        {
            for (const auto cell : cells)
            {
                paths.insert(cell);
            }
        }

        // One tick: run, then commit, as SystemScheduler would.
        void tick()
        {
            system.update(world, 0);
            world.commit();
        }

        ::testing::NiceMock<MockLogger> logger;
        World world{logger};
        PathIndex paths;
        WalkerSystem system{paths};
    };
} // namespace

TEST_F(WalkerSystemTest, Update_AdvancesOneCellAlongAStraightPath)
{
    layPath({{.x = 0, .y = 0}, {.x = 1, .y = 0}, {.x = 2, .y = 0}});
    const auto walker = addWalker(Cell{.x = 0, .y = 0}, Direction::East);

    tick();

    EXPECT_EQ(world.get<Cell>(walker), (Cell{.x = 1, .y = 0}));
    EXPECT_EQ(world.get<Walker>(walker).facing, Direction::East);
}

TEST_F(WalkerSystemTest, Update_AdvancesAgainOnTheFollowingTick)
{
    layPath({{.x = 0, .y = 0}, {.x = 1, .y = 0}, {.x = 2, .y = 0}});
    const auto walker = addWalker(Cell{.x = 0, .y = 0}, Direction::East);

    tick();
    tick();

    EXPECT_EQ(world.get<Cell>(walker), (Cell{.x = 2, .y = 0}));
}

TEST_F(WalkerSystemTest, Update_TakesTheRightArmAtATJunction)
{
    // Heading east, with the path continuing east and turning south.
    // South is to the right of east.
    layPath(
        {{.x = 0, .y = 0},
         {.x = 1, .y = 0},
         {.x = 2, .y = 0},
         {.x = 1, .y = 1}});
    const auto walker = addWalker(Cell{.x = 1, .y = 0}, Direction::East);

    tick();

    EXPECT_EQ(world.get<Cell>(walker), (Cell{.x = 1, .y = 1}));
    EXPECT_EQ(world.get<Walker>(walker).facing, Direction::South);
}

TEST_F(WalkerSystemTest, Update_ComesBackFromADeadEnd)
{
    layPath({{.x = 0, .y = 0}, {.x = 1, .y = 0}});
    const auto walker = addWalker(Cell{.x = 1, .y = 0}, Direction::East);

    tick();

    EXPECT_EQ(world.get<Cell>(walker), (Cell{.x = 0, .y = 0}));
    EXPECT_EQ(world.get<Walker>(walker).facing, Direction::West);
}

TEST_F(WalkerSystemTest, Update_LeavesAWalkerOnAnIsolatedTileWhereItIs)
{
    layPath({{.x = 4, .y = 4}});
    const auto walker = addWalker(Cell{.x = 4, .y = 4}, Direction::North);

    tick();

    EXPECT_EQ(world.get<Cell>(walker), (Cell{.x = 4, .y = 4}));
    EXPECT_EQ(world.get<Walker>(walker).facing, Direction::North);
}

TEST_F(WalkerSystemTest, Update_MovesEveryWalkerNotJustTheFirst)
{
    layPath({{.x = 0, .y = 0}, {.x = 1, .y = 0}, {.x = 2, .y = 0}});
    const auto first = addWalker(Cell{.x = 0, .y = 0}, Direction::East);
    const auto second = addWalker(Cell{.x = 1, .y = 0}, Direction::East);

    tick();

    EXPECT_EQ(world.get<Cell>(first), (Cell{.x = 1, .y = 0}));
    EXPECT_EQ(world.get<Cell>(second), (Cell{.x = 2, .y = 0}));
}

// The guarantee World's double buffering provides, asserted.
// Two walkers meeting head-on pass through each other.
TEST_F(WalkerSystemTest, Update_HidesEachWalkersMoveFromTheOthers)
{
    layPath({{.x = 0, .y = 0}, {.x = 1, .y = 0}});
    const auto eastbound = addWalker(Cell{.x = 0, .y = 0}, Direction::East);
    const auto westbound = addWalker(Cell{.x = 1, .y = 0}, Direction::West);

    tick();

    EXPECT_EQ(world.get<Cell>(eastbound), (Cell{.x = 1, .y = 0}));
    EXPECT_EQ(world.get<Cell>(westbound), (Cell{.x = 0, .y = 0}));
}

TEST_F(WalkerSystemTest, Update_LeavesAPathTileAlone)
{
    // A path entity has no Walker, so the view must not pick it up.
    layPath({{.x = 0, .y = 0}, {.x = 1, .y = 0}});
    const auto tile = world.create();
    world.add<Cell>(tile, Cell{.x = 0, .y = 0});
    world.commit();

    tick();

    EXPECT_EQ(world.get<Cell>(tile), (Cell{.x = 0, .y = 0}));
}

TEST_F(WalkerSystemTest, Update_DoesNothingWithNoWalkersAtAll)
{
    layPath({{.x = 0, .y = 0}});

    EXPECT_NO_THROW(tick());
}

// serve() on its own, without a world to put the two into.

TEST(ServeTest, AFoodWalkerFillsAFoodBuildingAndIsDrawnDown)
{
    auto walker = newlySpawned(WalkerKind::Food, Direction::East);
    auto house = newlyBuilt(BuildingKind::House);

    serve(walker, house);

    EXPECT_EQ(house.stock.held, house.stock.capacity);
    EXPECT_EQ(walker.carried, 100 - 90);
}

TEST(ServeTest, AWalkerNeverPushesABuildingPastItsCapacity)
{
    auto walker = newlySpawned(WalkerKind::Food, Direction::East);
    auto house = newlyBuilt(BuildingKind::House);
    house.stock.held = house.stock.capacity;

    serve(walker, house);

    EXPECT_EQ(house.stock.held, house.stock.capacity);
    EXPECT_EQ(walker.carried, 100);
}

TEST(ServeTest, AnEmptyWalkerLeavesABuildingAlone)
{
    auto walker = newlySpawned(WalkerKind::Food, Direction::East);
    walker.carried = 0;
    auto house = newlyBuilt(BuildingKind::House);

    serve(walker, house);

    EXPECT_EQ(house.stock.held, 10);
}

TEST(ServeTest, AWaterWalkerLeavesAFoodBuildingAlone)
{
    auto walker = newlySpawned(WalkerKind::Water, Direction::East);
    auto house = newlyBuilt(BuildingKind::House);

    serve(walker, house);

    EXPECT_EQ(house.stock.held, 10);
    EXPECT_EQ(walker.carried, 100);
}

TEST(ServeTest, AWaterWalkerFillsWhatStocksWater)
{
    auto walker = newlySpawned(WalkerKind::Water, Direction::East);
    auto well = newlyBuilt(BuildingKind::WaterSource);
    ASSERT_EQ(well.stock.resource, Resource::Water);

    serve(walker, well);

    EXPECT_EQ(well.stock.held, well.stock.capacity);
}

TEST(ServeTest, AFiremanTakesTheFireRiskDownAndLeavesTheRest)
{
    auto fireman = newlySpawned(WalkerKind::Fireman, Direction::East);
    auto house = newlyBuilt(BuildingKind::House);
    house.fireRisk = kMaxRisk - 1;
    house.collapseRisk = kMaxRisk - 1;

    serve(fireman, house);

    EXPECT_EQ(house.fireRisk, kMaxRisk - 1 - kRiskRelief);
    EXPECT_EQ(house.collapseRisk, kMaxRisk - 1);
    EXPECT_EQ(house.stock.held, 10);
}

TEST(ServeTest, AnArchitectTakesTheCollapseRiskDownAndLeavesTheRest)
{
    auto architect = newlySpawned(WalkerKind::Architect, Direction::East);
    auto house = newlyBuilt(BuildingKind::House);
    house.fireRisk = kMaxRisk - 1;
    house.collapseRisk = kMaxRisk - 1;

    serve(architect, house);

    EXPECT_EQ(house.collapseRisk, kMaxRisk - 1 - kRiskRelief);
    EXPECT_EQ(house.fireRisk, kMaxRisk - 1);
}

// Relief never pushes a risk below nothing.
TEST(ServeTest, RiskNeverGoesBelowZero)
{
    auto fireman = newlySpawned(WalkerKind::Fireman, Direction::East);
    auto architect = newlySpawned(WalkerKind::Architect, Direction::East);
    auto house = newlyBuilt(BuildingKind::House);

    serve(fireman, house);
    serve(architect, house);

    EXPECT_EQ(house.fireRisk, 0);
    EXPECT_EQ(house.collapseRisk, 0);
}

// And the same rules once a world is holding the two.

TEST_F(WalkerSystemTest, Update_TopsUpABuildingBesideThePathCell)
{
    layPath({{.x = 0, .y = 0}, {.x = 1, .y = 0}});
    const auto walker =
        addCarrier(Cell{.x = 0, .y = 0}, Direction::East, WalkerKind::Food);
    const auto house =
        addBuilding(Cell{.x = 0, .y = 1}, newlyBuilt(BuildingKind::House));

    tick();

    EXPECT_EQ(world.get<Building>(house).stock.held, 100);
    EXPECT_EQ(world.get<Walker>(walker).carried, 10);
}

TEST_F(WalkerSystemTest, Update_LeavesABuildingDiagonallyOffThePathAlone)
{
    layPath({{.x = 0, .y = 0}, {.x = 1, .y = 0}});
    (void)addCarrier(
        Cell{.x = 0, .y = 0}, Direction::East, WalkerKind::Food);
    const auto house =
        addBuilding(Cell{.x = 1, .y = 1}, newlyBuilt(BuildingKind::House));

    tick();

    EXPECT_EQ(world.get<Building>(house).stock.held, 10);
}

// Two walkers, one building, one tick: neither delivery may be lost.
TEST_F(WalkerSystemTest, Update_AddsUpTwoDeliveriesInOneTick)
{
    layPath({{.x = 0, .y = 0}, {.x = 1, .y = 0}});
    const auto house =
        addBuilding(Cell{.x = 0, .y = 1}, newlyBuilt(BuildingKind::House));

    for (int walker = 0; walker < 2; ++walker)
    {
        const auto entity = world.create();
        world.add<Cell>(entity, Cell{.x = 0, .y = 0});
        world.add<Walker>(
            entity,
            Walker{
                .facing = Direction::East,
                .kind = WalkerKind::Food,
                .carried = 5,
                .stepsTaken = 0});
    }
    world.commit();

    tick();

    EXPECT_EQ(world.get<Building>(house).stock.held, 20);
}

// Standing still is not being idle.
TEST_F(WalkerSystemTest, Update_ServesFromACellWithNowhereToGo)
{
    layPath({{.x = 0, .y = 0}});
    (void)addCarrier(
        Cell{.x = 0, .y = 0}, Direction::East, WalkerKind::Food);
    const auto house =
        addBuilding(Cell{.x = 0, .y = 1}, newlyBuilt(BuildingKind::House));

    tick();

    EXPECT_EQ(world.get<Building>(house).stock.held, 100);
}

TEST_F(WalkerSystemTest, Update_CountsEveryStepItTakes)
{
    layPath({{.x = 0, .y = 0}, {.x = 1, .y = 0}, {.x = 2, .y = 0}});
    const auto walker =
        addCarrier(Cell{.x = 0, .y = 0}, Direction::East, WalkerKind::Food);

    tick();
    tick();

    EXPECT_EQ(world.get<Walker>(walker).stepsTaken, 2);
}

// The distance cap: a walker that has gone far enough turns for home.
TEST_F(WalkerSystemTest, Update_TurnsForHomeAtTheDistanceLimit)
{
    std::vector<Cell> corridor;
    for (std::int32_t x = 0; x <= kMaxWalkDistance + 2; ++x)
    {
        corridor.push_back(Cell{.x = x, .y = 0});
    }
    layPath(corridor);
    const auto walker = addCarrier(
        Cell{.x = 0, .y = 0}, Direction::East, WalkerKind::Food);

    for (std::int32_t taken = 0; taken < kMaxWalkDistance; ++taken)
    {
        tick();
    }

    ASSERT_EQ((world.view<Walker, Cell>().size()), 1U);
    ASSERT_EQ(world.get<Cell>(walker), (Cell{.x = kMaxWalkDistance, .y = 0}));

    tick();

    // Still there, one cell back the way it came, facing that way.
    ASSERT_EQ((world.view<Walker, Cell>().size()), 1U);
    EXPECT_EQ(
        world.get<Cell>(walker),
        (Cell{.x = kMaxWalkDistance - 1, .y = 0}));
    EXPECT_EQ(world.get<Walker>(walker).facing, Direction::West);
}

TEST_F(WalkerSystemTest, Update_WalksAllTheWayHomeAndThenDespawns)
{
    layPath(
        {{.x = 0, .y = 0},
         {.x = 1, .y = 0},
         {.x = 2, .y = 0},
         {.x = 3, .y = 0}});
    (void)addReturner(
        Cell{.x = 3, .y = 0}, Direction::East, Cell{.x = 0, .y = 0});

    // Three steps home, then gone on the tick it has arrived.
    tick();
    tick();
    tick();

    ASSERT_EQ((world.view<Walker, Cell>().size()), 1U);

    tick();

    EXPECT_EQ((world.view<Walker, Cell>().size()), 0U);
}

TEST_F(WalkerSystemTest, Update_DespawnsAtTheLimitWhenTheRoadIsCut)
{
    // (3,0) and (2,0) are a stub with nothing joining them to home.
    layPath({{.x = 0, .y = 0}, {.x = 2, .y = 0}, {.x = 3, .y = 0}});
    (void)addReturner(
        Cell{.x = 3, .y = 0}, Direction::East, Cell{.x = 0, .y = 0});

    tick();

    EXPECT_EQ((world.view<Walker, Cell>().size()), 0U);
}

TEST_F(WalkerSystemTest, Update_TakesTheShorterWayHomeRoundACorner)
{
    // A U: the walker is at (2,0) and home is (0,0).
    // The only road between them runs down and back up through y = 1.
    layPath(
        {{.x = 0, .y = 0},
         {.x = 0, .y = 1},
         {.x = 1, .y = 1},
         {.x = 2, .y = 1},
         {.x = 2, .y = 0}});
    const auto walker = addReturner(
        Cell{.x = 2, .y = 0}, Direction::North, Cell{.x = 0, .y = 0});

    tick();

    EXPECT_EQ(world.get<Cell>(walker), (Cell{.x = 2, .y = 1}));
    EXPECT_EQ(world.get<Walker>(walker).facing, Direction::South);
}

TEST_F(WalkerSystemTest, Update_ServesNothingOnTheWayHome)
{
    layPath({{.x = 0, .y = 0}, {.x = 1, .y = 0}, {.x = 2, .y = 0}});
    const auto house = addBuilding(
        Cell{.x = 1, .y = 1}, newlyBuilt(BuildingKind::House));
    const auto walker = addReturner(
        Cell{.x = 2, .y = 0}, Direction::East, Cell{.x = 0, .y = 0});
    auto loaded = world.get<Walker>(walker);
    loaded.carried = 50;
    world.set<Walker>(walker, loaded);
    world.commit();

    const auto before = world.get<Building>(house).stock.held;

    tick();

    EXPECT_EQ(world.get<Building>(house).stock.held, before);
}

TEST_F(WalkerSystemTest, Update_DespawnsAWalkerAlreadyHomeAtTheLimit)
{
    layPath({{.x = 0, .y = 0}, {.x = 1, .y = 0}});
    (void)addReturner(
        Cell{.x = 0, .y = 0}, Direction::East, Cell{.x = 0, .y = 0});

    tick();

    EXPECT_EQ((world.view<Walker, Cell>().size()), 0U);
}

namespace
{
    // One whole life of one walker, as a trace two runs can be
    // compared on.
    // Everything a replay has to reproduce is in here: where it was
    // each tick, which way it faced, and the tick it left the world.
    struct Step
    {
        Cell at;
        Direction facing;

        [[nodiscard]] bool operator==(const Step &other) const = default;
    };

    // Run the same board twice over, in worlds that share nothing.
    [[nodiscard]] std::vector<Step> traceOf(
        const std::vector<Cell> &roads,
        Cell from,
        Cell origin,
        std::int32_t ticks)
    {
        PathIndex paths;
        for (const auto cell : roads)
        {
            (void)paths.insert(cell);
        }

        ::testing::NiceMock<MockLogger> logger;
        World world{logger};
        WalkerSystem system(paths);

        const auto entity = world.create();
        world.add<Cell>(entity, from);
        world.add<Walker>(
            entity,
            Walker{
                .facing = Direction::East,
                .stepsTaken = kMaxWalkDistance,
                .origin = origin});
        world.commit();

        std::vector<Step> trace;

        for (std::int32_t tick = 0; tick < ticks; ++tick)
        {
            system.update(world, tick);
            world.commit();

            if (world.view<Walker, Cell>().size() == 0U)
            {
                break;
            }

            trace.push_back(
                Step{
                    .at = world.get<Cell>(entity),
                    .facing = world.get<Walker>(entity).facing});
        }

        return trace;
    }
} // namespace

// Determinism, which is the point of doing the search this way: the
// route home is a function of the roads and the two cells, and of
// nothing that varies between runs.
TEST(WalkerReplayDeterminismTest, WalkingHomeReplaysStepForStep)
{
    const std::vector<Cell> roads{
        {.x = 0, .y = 0},
        {.x = 1, .y = 0},
        {.x = 2, .y = 0},
        {.x = 2, .y = 1},
        {.x = 1, .y = 1},
        {.x = 0, .y = 1}};

    const auto first =
        traceOf(roads, Cell{.x = 2, .y = 0}, Cell{.x = 0, .y = 0}, 8);
    const auto second =
        traceOf(roads, Cell{.x = 2, .y = 0}, Cell{.x = 0, .y = 0}, 8);

    // Two equally cheap ways home, so a tie was broken to get here.
    ASSERT_EQ(first.size(), 2U);
    EXPECT_EQ(first, second);
    EXPECT_EQ(
        first.front(),
        (Step{.at = Cell{.x = 1, .y = 0}, .facing = Direction::West}));
}

TEST(WalkerReplayDeterminismTest, ACutRoadEndsItOnTheSameTickBothRuns)
{
    // (2,0) joins nothing: the road home was never laid.
    const std::vector<Cell> roads{
        {.x = 0, .y = 0}, {.x = 2, .y = 0}, {.x = 3, .y = 0}};

    const auto first =
        traceOf(roads, Cell{.x = 3, .y = 0}, Cell{.x = 0, .y = 0}, 8);
    const auto second =
        traceOf(roads, Cell{.x = 3, .y = 0}, Cell{.x = 0, .y = 0}, 8);

    EXPECT_TRUE(first.empty());
    EXPECT_EQ(first, second);
}
