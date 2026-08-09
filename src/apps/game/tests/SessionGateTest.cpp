#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstddef>

#include <antwika/ecs/ISystem.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/ecs/mocks/MockSystem.hpp>
#include <antwika/engine/Events.hpp>
#include <antwika/event/Event.hpp>
#include <antwika/event/ITickEventSink.hpp>
#include <antwika/event/TickEvent.hpp>
#include <antwika/event/mocks/MockTickEventSink.hpp>
#include <antwika/log/mocks/MockLogger.hpp>

#include "antwika/game/AppMode.hpp"
#include "antwika/game/ModeGatedSink.hpp"
#include "antwika/game/SessionGatedSystem.hpp"

using antwika::ecs::ISystem;
using antwika::ecs::World;
using antwika::ecs::mocks::MockSystem;
using antwika::event::Event;
using antwika::event::ITickEventSink;
using antwika::event::TickEvent;
using antwika::event::mocks::MockTickEventSink;
using antwika::game::AppMode;
using antwika::game::AppModeState;
using antwika::game::ModeGatedSink;
using antwika::game::SessionGatedSystem;
using antwika::log::mocks::MockLogger;
using ::testing::_;
using ::testing::NiceMock;

namespace
{
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
}

TEST(ModeGatedSinkTest, Handle_PassesInputInItsOwnMode)
{
    NiceMock<MockTickEventSink> inner;
    EXPECT_CALL(inner, handle(_)).Times(1);
    const AppModeState mode{AppMode::CityMap};
    ModeGatedSink gate(inner, mode, AppMode::CityMap);

    gate.handle(click());
}

TEST(ModeGatedSinkTest, Handle_DropsInputInAnotherMode)
{
    NiceMock<MockTickEventSink> inner;
    EXPECT_CALL(inner, handle(_)).Times(0);
    const AppModeState mode{AppMode::MainMenu};
    ModeGatedSink gate(inner, mode, AppMode::CityMap);

    gate.handle(click());
}

TEST(ModeGatedSinkTest, Handle_AlwaysPassesAnEngineTick)
{
    NiceMock<MockTickEventSink> inner;
    EXPECT_CALL(inner, handle(_)).Times(1);
    const AppModeState mode{AppMode::MainMenu};
    ModeGatedSink gate(inner, mode, AppMode::CityMap);

    gate.handle(tick());
}

TEST(SessionGatedSystemTest, Update_RunsWhileACityIsUp)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    NiceMock<MockSystem> inner;
    EXPECT_CALL(inner, update(_, _)).Times(1);
    const AppModeState mode{AppMode::CityMap};
    SessionGatedSystem gate(inner, mode);

    gate.update(world, 0);
}

TEST(SessionGatedSystemTest, Update_RunsWhileTheWorldMapIsUp)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    NiceMock<MockSystem> inner;
    EXPECT_CALL(inner, update(_, _)).Times(1);
    const AppModeState mode{AppMode::WorldMap};
    SessionGatedSystem gate(inner, mode);

    gate.update(world, 0);
}

TEST(SessionGatedSystemTest, Update_StagesNothingOnTheMainMenu)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    NiceMock<MockSystem> inner;
    EXPECT_CALL(inner, update(_, _)).Times(0);
    const AppModeState mode{AppMode::MainMenu};
    SessionGatedSystem gate(inner, mode);

    gate.update(world, 0);
}

TEST(SessionGatedSystemTest, Update_StagesNothingOnTheSavePicker)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    NiceMock<MockSystem> inner;
    EXPECT_CALL(inner, update(_, _)).Times(0);
    const AppModeState mode{AppMode::SaveLoad};
    SessionGatedSystem gate(inner, mode);

    gate.update(world, 0);
}
