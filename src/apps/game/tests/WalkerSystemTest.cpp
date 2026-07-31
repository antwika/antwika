#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <vector>

#include <antwika/ecs/World.hpp>
#include <antwika/log/mocks/MockLogger.hpp>

#include "antwika/game/Building.hpp"
#include "antwika/game/Cell.hpp"
#include "antwika/game/Direction.hpp"
#include "antwika/game/GridExtent.hpp"
#include "antwika/game/PathIndex.hpp"
#include "antwika/game/Walker.hpp"
#include "antwika/game/WalkerSystem.hpp"

using antwika::ecs::Entity;
using antwika::ecs::World;
using antwika::game::Cell;
using antwika::game::Direction;
using antwika::game::PathIndex;
using antwika::game::Walker;
using antwika::game::WalkerSystem;
using antwika::log::mocks::MockLogger;

namespace
{
    constexpr antwika::game::GridExtent kExtent{
        .width = 16, .height = 16};

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
        WalkerSystem system{paths, kExtent};
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

TEST_F(WalkerSystemTest, Update_StaysPutOnTheTickAfterAStep)
{
    layPath({{.x = 0, .y = 0}, {.x = 1, .y = 0}, {.x = 2, .y = 0}});
    const auto walker = addWalker(Cell{.x = 0, .y = 0}, Direction::East);

    tick();
    tick();

    // A step takes two ticks, so the second one only counts down.
    EXPECT_EQ(world.get<Cell>(walker), (Cell{.x = 1, .y = 0}));
    EXPECT_EQ(world.get<Walker>(walker).facing, Direction::East);
}

TEST_F(WalkerSystemTest, Update_AdvancesAgainOnEveryOtherTick)
{
    layPath({{.x = 0, .y = 0}, {.x = 1, .y = 0}, {.x = 2, .y = 0}});
    const auto walker = addWalker(Cell{.x = 0, .y = 0}, Direction::East);

    tick();
    tick();
    tick();

    EXPECT_EQ(world.get<Cell>(walker), (Cell{.x = 2, .y = 0}));
}

// The cadence is the walker's own, not a modulus on the tick number.
TEST_F(WalkerSystemTest, Update_KeepsEachWalkersOwnCadence)
{
    layPath(
        {{.x = 0, .y = 0},
         {.x = 1, .y = 0},
         {.x = 2, .y = 0},
         {.x = 3, .y = 0},
         {.x = 4, .y = 0}});
    const auto early = addWalker(Cell{.x = 0, .y = 0}, Direction::East);

    tick();

    // Dropped a tick after the first, so it steps on the other ticks.
    const auto late = addWalker(Cell{.x = 3, .y = 0}, Direction::East);

    tick();
    tick();

    EXPECT_EQ(world.get<Cell>(early), (Cell{.x = 2, .y = 0}));
    EXPECT_EQ(world.get<Cell>(late), (Cell{.x = 4, .y = 0}));
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

TEST_F(WalkerSystemTest, Update_RecordsTheCellAStepBeganFrom)
{
    layPath({{.x = 0, .y = 0}, {.x = 1, .y = 0}, {.x = 2, .y = 0}});
    const auto walker = addWalker(Cell{.x = 0, .y = 0}, Direction::East);

    tick();

    EXPECT_EQ(world.get<Walker>(walker).from, (Cell{.x = 0, .y = 0}));
}

TEST_F(WalkerSystemTest, Update_KeepsTheStartCellWhileCountingDown)
{
    // The countdown branch rebuilds the whole walker.
    // So this catches it dropping the field it does not name.
    layPath({{.x = 0, .y = 0}, {.x = 1, .y = 0}, {.x = 2, .y = 0}});
    const auto walker = addWalker(Cell{.x = 0, .y = 0}, Direction::East);

    tick();
    tick();

    EXPECT_EQ(world.get<Walker>(walker).ticksUntilStep, 0U);
    EXPECT_EQ(world.get<Walker>(walker).from, (Cell{.x = 0, .y = 0}));
}

TEST_F(WalkerSystemTest, Update_MovesTheStartCellOnEveryStep)
{
    layPath({{.x = 0, .y = 0}, {.x = 1, .y = 0}, {.x = 2, .y = 0}});
    const auto walker = addWalker(Cell{.x = 0, .y = 0}, Direction::East);

    tick();
    tick();
    tick();

    EXPECT_EQ(world.get<Cell>(walker), (Cell{.x = 2, .y = 0}));
    EXPECT_EQ(world.get<Walker>(walker).from, (Cell{.x = 1, .y = 0}));
}

TEST_F(WalkerSystemTest, Update_LeavesNoStartCellForAWalkerThatCannotMove)
{
    // Nothing to step onto, so it never came from anywhere.
    layPath({{.x = 5, .y = 5}});
    const auto walker = addWalker(Cell{.x = 5, .y = 5}, Direction::East);

    tick();

    EXPECT_FALSE(world.get<Walker>(walker).from.has_value());
}

TEST_F(WalkerSystemTest, Update_TiresAWalkerOneStepAtATime)
{
    layPath({{.x = 0, .y = 0}, {.x = 1, .y = 0}, {.x = 2, .y = 0}});
    const auto walker = addWalker(Cell{.x = 0, .y = 0}, Direction::East);

    tick();

    EXPECT_EQ(
        world.get<Walker>(walker).stepsUntilHome,
        antwika::game::kRoamingSteps - 1);
}

TEST_F(WalkerSystemTest, Update_DoesNotTireAWalkerThatCannotMove)
{
    // It has not walked, so it has not tired.
    layPath({{.x = 5, .y = 5}});
    const auto walker = addWalker(Cell{.x = 5, .y = 5}, Direction::East);

    tick();

    EXPECT_EQ(
        world.get<Walker>(walker).stepsUntilHome,
        antwika::game::kRoamingSteps);
}

TEST_F(WalkerSystemTest, Update_RemovesATiredWalkerThatNobodySent)
{
    layPath({{.x = 0, .y = 0}, {.x = 1, .y = 0}});

    const auto walker = world.create();
    world.add<Cell>(walker, Cell{.x = 0, .y = 0});
    world.add<Walker>(walker, Walker{.stepsUntilHome = 0});
    world.commit();

    tick();

    EXPECT_FALSE(world.alive(walker));
}

TEST_F(WalkerSystemTest, Update_WalksATiredWalkerBackTowardsItsBuilding)
{
    layPath({{.x = 1, .y = 0}, {.x = 2, .y = 0}, {.x = 3, .y = 0}});

    const auto home = world.create();
    world.add<Cell>(home, Cell{.x = 0, .y = 0});
    world.add<antwika::game::Building>(home, antwika::game::Building{});

    const auto walker = world.create();
    world.add<Cell>(walker, Cell{.x = 3, .y = 0});
    world.add<Walker>(
        walker,
        Walker{
            .facing = Direction::East,
            .stepsUntilHome = 0,
            .home = home});
    world.commit();

    // Its home is west, so it turns round rather than carrying on.
    tick();

    EXPECT_EQ(world.get<Cell>(walker), (Cell{.x = 2, .y = 0}));
    EXPECT_EQ(world.get<Walker>(walker).facing, Direction::West);
}

TEST_F(WalkerSystemTest, Update_RemovesAWalkerAsItReachesItsBuilding)
{
    layPath({{.x = 1, .y = 0}});

    const auto home = world.create();
    world.add<Cell>(home, Cell{.x = 0, .y = 0});
    world.add<antwika::game::Building>(home, antwika::game::Building{});

    const auto walker = world.create();
    world.add<Cell>(walker, Cell{.x = 1, .y = 0});
    world.add<Walker>(walker, Walker{.stepsUntilHome = 0, .home = home});
    world.commit();

    tick();

    EXPECT_FALSE(world.alive(walker));
}

TEST_F(WalkerSystemTest, Update_RemovesATiredWalkerWhoseBuildingIsGone)
{
    layPath({{.x = 1, .y = 0}});

    const auto home = world.create();
    world.add<Cell>(home, Cell{.x = 0, .y = 0});
    world.add<antwika::game::Building>(home, antwika::game::Building{});

    const auto walker = world.create();
    world.add<Cell>(walker, Cell{.x = 1, .y = 0});
    world.add<Walker>(
        walker, Walker{.stepsUntilHome = 0, .home = home});
    world.commit();

    world.destroy(home);
    world.commit();

    tick();

    EXPECT_FALSE(world.alive(walker));
}

TEST_F(WalkerSystemTest, Update_RemovesATiredWalkerWalledOffFromItsHome)
{
    // No road runs between the two, so there is no route back.
    layPath({{.x = 5, .y = 5}});

    const auto home = world.create();
    world.add<Cell>(home, Cell{.x = 0, .y = 0});
    world.add<antwika::game::Building>(home, antwika::game::Building{});

    const auto walker = world.create();
    world.add<Cell>(walker, Cell{.x = 5, .y = 5});
    world.add<Walker>(
        walker, Walker{.stepsUntilHome = 0, .home = home});
    world.commit();

    tick();

    EXPECT_FALSE(world.alive(walker));
}

TEST_F(WalkerSystemTest, Update_KeepsATiredWalkerCountingDownBetweenSteps)
{
    layPath({{.x = 1, .y = 0}, {.x = 2, .y = 0}, {.x = 3, .y = 0}});

    const auto home = world.create();
    world.add<Cell>(home, Cell{.x = 0, .y = 0});
    world.add<antwika::game::Building>(home, antwika::game::Building{});

    const auto walker = world.create();
    world.add<Cell>(walker, Cell{.x = 3, .y = 0});
    world.add<Walker>(
        walker, Walker{.stepsUntilHome = 0, .home = home});
    world.commit();

    tick();
    tick();

    // One step taken, then a tick spent waiting rather than a second.
    EXPECT_EQ(world.get<Cell>(walker), (Cell{.x = 2, .y = 0}));
    EXPECT_TRUE(world.alive(walker));
}
