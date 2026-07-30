#include "antwika/replay/TickPacer.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <chrono>
#include <vector>

#include <antwika/log/mocks/MockLogger.hpp>
#include <antwika/time/fakes/FakeSleeper.hpp>

using antwika::ecs::World;
using antwika::replay::TickPacer;
using antwika::log::mocks::MockLogger;
using antwika::time::fakes::FakeSleeper;
using ::testing::NiceMock;
using namespace std::chrono_literals;

namespace
{
    // Any component at all, to have something the pacer could disturb.
    struct Marker
    {
        int value = 0;

        [[nodiscard]] bool operator==(const Marker &other) const = default;
    };
} // namespace

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
    world.add<Marker>(entity, Marker{.value = 3});
    world.commit();

    FakeSleeper sleeper;
    TickPacer pacer(sleeper, 1ms);

    pacer.update(world, 0);
    world.commit();

    EXPECT_EQ(world.get<Marker>(entity), (Marker{.value = 3}));

    // Removing and destroying instantiate the rest of this storage.
    // A type used only here would leave its unused members uncovered.
    // The coverage gate counts those as functions.
    // Destroying is what reaches the pool's own removal callback.
    world.remove<Marker>(entity);
    world.commit();

    EXPECT_FALSE(world.has<Marker>(entity));

    const auto second = world.create();
    world.add<Marker>(second, Marker{.value = 7});
    world.commit();

    world.destroy(second);
    world.commit();

    EXPECT_FALSE(world.has<Marker>(second));
}
