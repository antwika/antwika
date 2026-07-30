#include "antwika/game/TickPacer.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <chrono>
#include <vector>

#include <antwika/log/mocks/MockLogger.hpp>
#include <antwika/time/fakes/FakeSleeper.hpp>

#include "antwika/game/Cell.hpp"
#include "antwika/game/Path.hpp"
#include "antwika/game/Walker.hpp"

using antwika::ecs::World;
using antwika::game::Cell;
using antwika::game::Path;
using antwika::game::Walker;
using antwika::game::TickPacer;
using antwika::log::mocks::MockLogger;
using antwika::time::fakes::FakeSleeper;
using ::testing::NiceMock;
using namespace std::chrono_literals;

// Nothing here measures elapsed time.
// The pacer only asks a sleeper to wait, so the ask is the behaviour.
// Asserting it costs no wall clock at all.
TEST(TickPacerTest, Update_AsksToWaitTheConfiguredInterval)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    FakeSleeper sleeper;
    TickPacer pacer(sleeper, 2ms);

    pacer.update(world, 0);

    EXPECT_EQ(
        sleeper.requested(), (std::vector<std::chrono::milliseconds>{2ms}));
}

TEST(TickPacerTest, Update_WaitsOncePerTick)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    FakeSleeper sleeper;
    TickPacer pacer(sleeper, 5ms);

    pacer.update(world, 0);
    pacer.update(world, 1);
    pacer.update(world, 2);

    EXPECT_EQ(sleeper.requested().size(), 3);
    EXPECT_EQ(sleeper.total(), 15ms);
}

TEST(TickPacerTest, Update_AsksForNothingWhenGivenAZeroInterval)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    FakeSleeper sleeper;
    TickPacer pacer(sleeper, std::chrono::milliseconds::zero());

    pacer.update(world, 0);

    EXPECT_EQ(sleeper.total(), 0ms);
}

// The pacer is an observer like any other, so it must observe nothing.
TEST(TickPacerTest, Update_LeavesTheWorldUnchanged)
{
    NiceMock<MockLogger> logger;
    World world(logger);

    const auto entity = world.create();
    world.add<Cell>(entity, Cell{.x = 3, .y = 4});
    world.add<Walker>(entity, Walker{});
    world.add<Path>(entity, Path{});
    world.commit();

    FakeSleeper sleeper;
    TickPacer pacer(sleeper, 1ms);

    pacer.update(world, 0);
    world.commit();

    EXPECT_EQ(world.get<Cell>(entity), (Cell{.x = 3, .y = 4}));
    EXPECT_TRUE(world.has<Walker>(entity));
    EXPECT_TRUE(world.has<Path>(entity));
}
