#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <chrono>
#include <vector>

#include <antwika/ecs/OpenPhase.hpp>
#include <antwika/log/mocks/MockLogger.hpp>
#include <antwika/time/fakes/FakeSleeper.hpp>

#include "antwika/system/TickPacer.hpp"

using antwika::ecs::OpenPhase;
using antwika::ecs::World;
using antwika::system::TickPacer;
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
    {
        const OpenPhase phase(world);

        world.add<Marker>(entity, Marker{.value = 3});
    }

    FakeSleeper sleeper;
    TickPacer pacer(sleeper, 1ms);

    {
        const OpenPhase phase(world);

        pacer.update(world, 0);
    }

    EXPECT_EQ(world.get<Marker>(entity), (Marker{.value = 3}));
}

TEST(TickPacerTest, Update_LeavesTheWorldStillEditable)
{
    NiceMock<MockLogger> logger;
    World world(logger);

    const auto entity = world.create();
    {
        const OpenPhase phase(world);

        world.add<Marker>(entity, Marker{.value = 3});
    }

    FakeSleeper sleeper;
    TickPacer pacer(sleeper, 1ms);
    pacer.update(world, 0);

    {
        const OpenPhase phase(world);

        world.remove<Marker>(entity);
    }

    EXPECT_FALSE(world.has<Marker>(entity));

    const auto second = world.create();
    {
        const OpenPhase phase(world);

        world.add<Marker>(second, Marker{.value = 7});
    }

    ASSERT_EQ(world.get<Marker>(second), (Marker{.value = 7}));

    {
        const OpenPhase phase(world);

        world.destroy(second);
    }

    EXPECT_FALSE(world.has<Marker>(second));
}
