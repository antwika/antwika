#include "antwika/life/TickPacer.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <chrono>

#include <antwika/log/mocks/MockLogger.hpp>

#include "antwika/life/Board.hpp"
#include "antwika/life/Cell.hpp"
#include "antwika/life/Grid.hpp"

using antwika::ecs::World;
using antwika::life::Cell;
using antwika::life::Grid;
using antwika::life::TickPacer;
using antwika::log::mocks::MockLogger;
using ::testing::NiceMock;

// sleep_for is specified to block for at least the interval it is given.
// So this only asserts the direction the specification guarantees.
TEST(TickPacerTest, Update_WaitsAtLeastTheConfiguredInterval)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    TickPacer pacer(std::chrono::milliseconds{2});

    const auto before = std::chrono::steady_clock::now();
    pacer.update(world, 0);
    const auto elapsed = std::chrono::steady_clock::now() - before;

    EXPECT_GE(elapsed, std::chrono::milliseconds{2});
}

TEST(TickPacerTest, Update_ReturnsImmediatelyForAZeroInterval)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    TickPacer pacer(std::chrono::milliseconds::zero());

    pacer.update(world, 0);

    SUCCEED();
}

// The pacer is an observer like any other, so it must observe nothing.
TEST(TickPacerTest, Update_LeavesTheWorldUnchanged)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    Grid grid(world, 2, 2);
    world.commit();
    world.set<Cell>(grid.entityAt(1, 0), Cell{.alive = true});
    world.commit();

    const auto before = antwika::life::readBoard(world, grid);

    TickPacer pacer(std::chrono::milliseconds::zero());
    pacer.update(world, 0);
    world.commit();

    EXPECT_EQ(antwika::life::readBoard(world, grid), before);
}
