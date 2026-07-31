#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstddef>

#include <antwika/ecs/ISystem.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/engine/Events.hpp>
#include <antwika/event/Event.hpp>
#include <antwika/event/ITickEventSink.hpp>
#include <antwika/event/TickEvent.hpp>
#include <antwika/log/mocks/MockLogger.hpp>

#include "antwika/game/AppMode.hpp"
#include "antwika/game/ModeGatedSink.hpp"
#include "antwika/game/ModeGatedSystem.hpp"

using antwika::ecs::ISystem;
using antwika::ecs::World;
using antwika::event::Event;
using antwika::event::ITickEventSink;
using antwika::event::TickEvent;
using antwika::game::AppMode;
using antwika::game::AppModeState;
using antwika::game::ModeGatedSink;
using antwika::game::ModeGatedSystem;
using antwika::log::mocks::MockLogger;
using ::testing::NiceMock;

namespace
{
    // A counter rather than a mock.
    // What is asserted is how many times something ran.
    class CountingSink final : public ITickEventSink
    {
    public:
        void handle(const TickEvent &) override
        {
            ++calls;
        }

        std::size_t calls = 0;
    };

    class CountingSystem final : public ISystem
    {
    public:
        void update(World &, antwika::time::Tick) override
        {
            ++calls;
        }

        std::size_t calls = 0;
    };

    [[nodiscard]] TickEvent tick()
    {
        return TickEvent{
            .tick = 0,
            .event = Event{.name = antwika::engine::events::kTick}};
    }

    [[nodiscard]] TickEvent click()
    {
        return TickEvent{
            .tick = 0, .event = Event{.name = "input.pointer_down"}};
    }
} // namespace

TEST(ModeGatedSinkTest, InputReachesTheSinkInItsOwnMode)
{
    CountingSink inner;
    const AppModeState mode{AppMode::Playing};
    ModeGatedSink gate(inner, mode, AppMode::Playing);

    gate.handle(click());

    EXPECT_EQ(inner.calls, 1U);
}

TEST(ModeGatedSinkTest, InputIsDroppedInAnyOtherMode)
{
    CountingSink inner;
    const AppModeState mode{AppMode::MainMenu};
    ModeGatedSink gate(inner, mode, AppMode::Playing);

    gate.handle(click());

    EXPECT_EQ(inner.calls, 0U);
}

// The carve-out the whole design rests on.
// A tick that stopped arriving would stop the renderer and the pacer.
TEST(ModeGatedSinkTest, EngineTickAlwaysReachesTheSink)
{
    CountingSink inner;
    const AppModeState mode{AppMode::MainMenu};
    ModeGatedSink gate(inner, mode, AppMode::Playing);

    gate.handle(tick());

    EXPECT_EQ(inner.calls, 1U);
}

TEST(ModeGatedSystemTest, TheSystemRunsInItsOwnMode)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    CountingSystem inner;
    const AppModeState mode{AppMode::Playing};
    ModeGatedSystem gate(inner, mode, AppMode::Playing);

    gate.update(world, 0);

    EXPECT_EQ(inner.calls, 1U);
}

TEST(ModeGatedSystemTest, TheSystemStagesNothingInAnyOtherMode)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    CountingSystem inner;
    const AppModeState mode{AppMode::MainMenu};
    ModeGatedSystem gate(inner, mode, AppMode::Playing);

    gate.update(world, 0);

    EXPECT_EQ(inner.calls, 0U);
}
