#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstdint>
#include <vector>

#include <antwika/log/mocks/MockLogger.hpp>

#include "antwika/life/Grid.hpp"
#include "antwika/life/Cell.hpp"

using antwika::ecs::Entity;
using antwika::ecs::World;
using antwika::life::Cell;
using antwika::life::Grid;
using antwika::log::mocks::MockLogger;
using ::testing::NiceMock;

TEST(GridTest, Ctor_MakesOneDeadCellPerCoordinate)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    Grid grid(world, 3, 2);
    world.commit();

    for (std::uint32_t y = 0; y < grid.height(); ++y)
    {
        for (std::uint32_t x = 0; x < grid.width(); ++x)
        {
            const auto cell = world.get<Cell>(grid.entityAt(x, y));
            EXPECT_EQ(cell, Cell{.alive = false});
        }
    }
}

TEST(GridTest, OperatorEquals_ComparesTheAliveState)
{
    EXPECT_EQ(Cell{.alive = true}, Cell{.alive = true});
    EXPECT_NE(Cell{.alive = true}, Cell{.alive = false});
}

TEST(GridTest, EntityAt_NumbersTheCellsRowByRow)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    Grid grid(world, 3, 2);
    world.commit();

    std::vector<Entity> addressed;

    for (std::uint32_t y = 0; y < grid.height(); ++y)
    {
        for (std::uint32_t x = 0; x < grid.width(); ++x)
        {
            addressed.push_back(grid.entityAt(x, y));
        }
    }

    const auto created = world.view<Cell>();

    ASSERT_EQ(addressed.size(), 6U);
    EXPECT_EQ(
        addressed,
        (std::vector<Entity>(created.begin(), created.end())));
}

TEST(GridTest, Commit_RemovesADestroyedCellsComponent)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    Grid grid(world, 2, 2);
    world.commit();

    const auto entity = grid.entityAt(0, 0);
    world.destroy(entity);
    world.commit();

    EXPECT_FALSE(world.alive(entity));
}

TEST(GridTest, Contains_IsTrueOnlyWithinBounds)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    Grid grid(world, 3, 2);

    EXPECT_TRUE(grid.contains(0, 0));
    EXPECT_TRUE(grid.contains(2, 1));
    EXPECT_FALSE(grid.contains(3, 0));
    EXPECT_FALSE(grid.contains(0, 2));
}
