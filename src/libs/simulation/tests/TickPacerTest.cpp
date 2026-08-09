#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <chrono>
#include <vector>

#include <antwika/log/mocks/MockLogger.hpp>
#include <antwika/time/fakes/FakeSleeper.hpp>

#include "antwika/simulation/TickPacer.hpp"

using antwika::ecs::World;
using antwika::simulation::TickPacer;
using antwika::log::mocks::MockLogger;
using antwika::time::fakes::FakeSleeper;
using ::testing::NiceMock;
using namespace std::chrono_literals;

namespace
{
    struct Marker final
    {
        int value = 0;

        [[nodiscard]] bool operator==(const Marker &other) const = default;
    };
}

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

    EXPECT_EQ(
        sleeper.requested(),
        (std::vector<std::chrono::milliseconds>{0ms}));
}

TEST(TickPacerTest, Update_LeavesTheWorldUnchanged)
{
    NiceMock<MockLogger> logger;
    World world(logger);

    const auto entity = world.create();
    world.add<Marker>(entity, Marker{.value = 3});
    world.commit();

    FakeSleeper sleeper;
    TickPacer pacer(sleeper, 1ms);

    pacer.update(world, 0);
    world.commit();

    EXPECT_EQ(world.get<Marker>(entity), (Marker{.value = 3}));
}

TEST(TickPacerTest, Update_LeavesTheWorldStillEditable)
{
    NiceMock<MockLogger> logger;
    World world(logger);

    const auto entity = world.create();
    world.add<Marker>(entity, Marker{.value = 3});
    world.commit();

    FakeSleeper sleeper;
    TickPacer pacer(sleeper, 1ms);
    pacer.update(world, 0);

    world.remove<Marker>(entity);
    world.commit();

    EXPECT_FALSE(world.has<Marker>(entity));

    const auto second = world.create();
    world.add<Marker>(second, Marker{.value = 7});
    world.commit();

    ASSERT_EQ(world.get<Marker>(second), (Marker{.value = 7}));

    world.destroy(second);
    world.commit();

    EXPECT_FALSE(world.has<Marker>(second));
}
