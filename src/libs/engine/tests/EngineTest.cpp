#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <antwika/log/mocks/MockLogger.hpp>
#include <antwika/log/Level.hpp>

#include "antwika/engine/Engine.hpp"
#include "antwika/engine/Events.hpp"
#include "antwika/event/mocks/MockEventDispatcher.hpp"

using antwika::engine::Engine;
using antwika::event::Event;
using antwika::event::mocks::MockEventDispatcher;
using antwika::log::Level;
using antwika::log::mocks::MockLogger;

TEST(EngineTest, Start_LogsStartupInfo)
{
    MockLogger mockLogger;
    MockEventDispatcher mockEventDispatcher;
    Engine engine(mockLogger, mockEventDispatcher);
    EXPECT_CALL(mockLogger, log(Level::Info, "Antwika engine started!"));
    engine.start();
}

TEST(EngineTest, Step_DispatchesBuiltInTickEvent)
{
    MockLogger mockLogger;
    MockEventDispatcher mockEventDispatcher;
    Engine engine(mockLogger, mockEventDispatcher);

    EXPECT_CALL(
        mockEventDispatcher,
        dispatch(Event{.name = antwika::engine::events::kTick}));

    engine.step(0);
}

TEST(EngineTest, Step_PropagatesExceptionWhenDispatcherDispatchFails)
{
    MockLogger mockLogger;
    MockEventDispatcher mockEventDispatcher;
    Engine engine(mockLogger, mockEventDispatcher);

    EXPECT_CALL(
        mockEventDispatcher,
        dispatch(Event{.name = antwika::engine::events::kTick}))
        .WillOnce(::testing::Throw(std::runtime_error("mockException")));

    EXPECT_THROW(engine.step(0), std::runtime_error);
}
