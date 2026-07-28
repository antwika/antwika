#include "antwika/life/Board.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <antwika/log/mocks/MockLogger.hpp>

#include "antwika/life/Cell.hpp"
#include "antwika/life/Grid.hpp"

using antwika::ecs::World;
using antwika::life::Board;
using antwika::life::Cell;
using antwika::life::Grid;
using antwika::log::mocks::MockLogger;
using ::testing::NiceMock;

TEST(BoardTest, ReadBoardSnapshotsEveryCellsAliveStateRowMajor)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    Grid grid(world, 2, 2);
    world.commit();

    world.set<Cell>(grid.entityAt(1, 0), Cell{.alive = true});
    world.commit();

    const auto board = antwika::life::readBoard(world, grid);

    EXPECT_EQ(board.width, 2U);
    EXPECT_EQ(board.height, 2U);
    EXPECT_EQ(board.alive, (std::vector<bool>{false, true, false, false}));
}
