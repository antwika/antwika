#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <vector>

#include <antwika/ecs/World.hpp>
#include <antwika/log/mocks/MockLogger.hpp>

#include "antwika/game/Cell.hpp"
#include "antwika/game/Direction.hpp"
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
