#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <antwika/log/mocks/MockLogger.hpp>

#include "antwika/life/Board.hpp"
#include "antwika/life/Cell.hpp"
#include "antwika/life/Grid.hpp"

using antwika::ecs::World;
using antwika::life::Board;
using antwika::life::Cell;
using antwika::life::Grid;
using antwika::log::mocks::MockLogger;
using ::testing::NiceMock;

TEST(BoardTest, ReadBoard_SnapshotsEveryCellRowMajor)
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

TEST(BoardTest, ReadBoardFromView_SnapshotsRowMajor)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    Grid grid(world, 3, 2);
    world.commit();

    world.set<Cell>(grid.entityAt(1, 0), Cell{.alive = true});
    world.set<Cell>(grid.entityAt(0, 1), Cell{.alive = true});
    world.commit();

    const auto board = antwika::life::readBoardFromView(world, 3, 2);

    EXPECT_EQ(board.width, 3U);
    EXPECT_EQ(board.height, 2U);
    EXPECT_EQ(
        board.alive,
        (std::vector<bool>{false, true, false, true, false, false}));
    EXPECT_EQ(board, antwika::life::readBoard(world, grid));
}

TEST(BoardTest, ReadBoardFromView_StopsAtTheDimensions)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    Grid grid(world, 3, 3);
    world.commit();

    world.set<Cell>(grid.entityAt(0, 0), Cell{.alive = true});
    world.commit();

    const auto board = antwika::life::readBoardFromView(world, 2, 1);

    EXPECT_EQ(board.alive, (std::vector<bool>{true, false}));
}

TEST(BoardTest, ReadBoardFromView_ReportsMissingAsDead)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    Grid grid(world, 1, 1);
    world.commit();

    world.set<Cell>(grid.entityAt(0, 0), Cell{.alive = true});
    world.commit();

    const auto board = antwika::life::readBoardFromView(world, 2, 2);

    EXPECT_EQ(board.alive, (std::vector<bool>{true, false, false, false}));
}

TEST(BoardTest, OperatorEquals_ComparesSizeAndAliveCells)
{
    const Board reference{
        .width = 2, .height = 2, .alive = {false, true, false, false}};
    const Board same{
        .width = 2, .height = 2, .alive = {false, true, false, false}};
    const Board differentWidth{
        .width = 3, .height = 2, .alive = {false, true, false, false}};
    const Board differentHeight{
        .width = 2, .height = 3, .alive = {false, true, false, false}};
    const Board differentAlive{
        .width = 2, .height = 2, .alive = {true, true, false, false}};

    EXPECT_EQ(reference, same);
    EXPECT_NE(reference, differentWidth);
    EXPECT_NE(reference, differentHeight);
    EXPECT_NE(reference, differentAlive);
}
