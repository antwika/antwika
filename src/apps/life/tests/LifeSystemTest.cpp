#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <antwika/log/mocks/MockLogger.hpp>

#include "antwika/life/LifeSystem.hpp"
#include "antwika/life/Board.hpp"
#include "antwika/life/Cell.hpp"
#include "antwika/life/Grid.hpp"

using antwika::ecs::World;
using antwika::life::Board;
using antwika::life::Cell;
using antwika::life::Grid;
using antwika::life::LifeSystem;
using antwika::log::mocks::MockLogger;
using ::testing::NiceMock;

namespace
{
    void setAlive(
        World &world, const Grid &grid, std::uint32_t x, std::uint32_t y)
    {
        world.set<Cell>(grid.entityAt(x, y), Cell{.alive = true});
    }
}

TEST(LifeSystemTest, Update_LeavesABlockAsAStillLife)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    Grid grid(world, 4, 4);
    world.commit();

    setAlive(world, grid, 1, 1);
    setAlive(world, grid, 2, 1);
    setAlive(world, grid, 1, 2);
    setAlive(world, grid, 2, 2);
    world.commit();

    const auto before = antwika::life::readBoard(world, grid);

    LifeSystem system(grid);
    system.update(world, 0);
    world.commit();

    EXPECT_EQ(antwika::life::readBoard(world, grid), before);
}

TEST(LifeSystemTest, Update_KillsALoneLiveCell)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    Grid grid(world, 3, 3);
    world.commit();

    setAlive(world, grid, 1, 1);
    world.commit();

    LifeSystem system(grid);
    system.update(world, 0);
    world.commit();

    EXPECT_FALSE(world.get<Cell>(grid.entityAt(1, 1)).alive);
}

TEST(LifeSystemTest, Update_RevivesADeadCellOnThreeNeighbours)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    Grid grid(world, 3, 3);
    world.commit();

    setAlive(world, grid, 0, 1);
    setAlive(world, grid, 1, 1);
    setAlive(world, grid, 2, 1);
    world.commit();

    LifeSystem system(grid);
    system.update(world, 0);
    world.commit();

    EXPECT_TRUE(world.get<Cell>(grid.entityAt(1, 0)).alive);
    EXPECT_TRUE(world.get<Cell>(grid.entityAt(1, 1)).alive);
    EXPECT_TRUE(world.get<Cell>(grid.entityAt(1, 2)).alive);
    EXPECT_FALSE(world.get<Cell>(grid.entityAt(0, 1)).alive);
    EXPECT_FALSE(world.get<Cell>(grid.entityAt(2, 1)).alive);
}
