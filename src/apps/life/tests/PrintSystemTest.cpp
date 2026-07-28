#include "antwika/life/PrintSystem.hpp"

#include <sstream>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <antwika/log/mocks/MockLogger.hpp>

#include "antwika/life/Cell.hpp"
#include "antwika/life/Grid.hpp"

using antwika::ecs::World;
using antwika::life::Cell;
using antwika::life::Grid;
using antwika::life::PrintSystem;
using antwika::log::mocks::MockLogger;
using ::testing::NiceMock;

TEST(PrintSystemTest, EachUpdateIsLabelledWithTheGivenTick)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    Grid grid(world, 2, 2);
    world.commit();

    std::ostringstream out;
    PrintSystem system(2, out);

    system.update(world, 0);
    system.update(world, 7);

    EXPECT_EQ(out.str(), "After tick 0:\n..\n..\nAfter tick 7:\n..\n..\n");
}

TEST(PrintSystemTest, PrintsEachAliveCellAsAHashAndDeadCellAsADot)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    Grid grid(world, 3, 2);
    world.commit();

    world.set<Cell>(grid.entityAt(1, 0), Cell{.alive = true});
    world.set<Cell>(grid.entityAt(0, 1), Cell{.alive = true});
    world.commit();

    std::ostringstream out;
    PrintSystem system(3, out);

    system.update(world, 0);

    EXPECT_EQ(out.str(), "After tick 0:\n.#.\n#..\n");
}
