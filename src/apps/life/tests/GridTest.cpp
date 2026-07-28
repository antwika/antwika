#include "antwika/life/Grid.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <antwika/log/mocks/MockLogger.hpp>

#include "antwika/life/Cell.hpp"

using antwika::ecs::World;
using antwika::life::Cell;
using antwika::life::Grid;
using antwika::log::mocks::MockLogger;
using ::testing::NiceMock;

TEST(GridTest, ConstructorCreatesOneDeadCellEntityPerCoordinate)
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

TEST(GridTest, EntityAtMapsDistinctCoordinatesToDistinctEntities)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    Grid grid(world, 2, 2);

    EXPECT_NE(grid.entityAt(0, 0), grid.entityAt(1, 0));
    EXPECT_NE(grid.entityAt(0, 0), grid.entityAt(0, 1));
    EXPECT_EQ(grid.entityAt(1, 1), grid.entityAt(1, 1));
}

TEST(GridTest, ContainsIsTrueOnlyWithinBounds)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    Grid grid(world, 3, 2);

    EXPECT_TRUE(grid.contains(0, 0));
    EXPECT_TRUE(grid.contains(2, 1));
    EXPECT_FALSE(grid.contains(3, 0));
    EXPECT_FALSE(grid.contains(0, 2));
}
