#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <antwika/log/mocks/MockLogger.hpp>
#include <antwika/log/Level.hpp>

#include "antwika/engine/Engine.hpp"
#include "antwika/engine/Events.hpp"
#include "antwika/event/mocks/MockEventDispatcher.hpp"
#include "antwika/event/mocks/MockEventQueue.hpp"

using antwika::engine::Engine;
using antwika::event::Event;
using antwika::event::mocks::MockEventDispatcher;
using antwika::event::mocks::MockEventQueue;
using antwika::log::Level;
using antwika::log::mocks::MockLogger;

TEST(EngineTest, Start_LogsStartupInfo)
{
    MockLogger mockLogger;
    MockEventQueue mockEventQueue;
    MockEventDispatcher mockEventDispatcher;
    Engine engine(mockLogger, mockEventQueue, mockEventDispatcher);
    EXPECT_CALL(mockLogger, log(Level::Info, "Antwika engine started!"));
    engine.start();
}

TEST(EngineTest, Step_DispatchesBuiltInTickEventBeforeProcessingQueue)
{
    MockLogger mockLogger;
    MockEventQueue mockEventQueue;
    MockEventDispatcher mockEventDispatcher;
    Engine engine(mockLogger, mockEventQueue, mockEventDispatcher);

    {
        ::testing::InSequence seq;
        EXPECT_CALL(mockLogger, log(Level::Info, "Engine step: tick 0"));
        EXPECT_CALL(mockEventDispatcher, dispatch(Event{.name = antwika::engine::events::kTick}));
        EXPECT_CALL(mockEventQueue, empty()).WillOnce(::testing::Return(true));
    }

    engine.step(0);
}

TEST(EngineTest, Step_ProcessesQueuedEventsForTheSteppedTick)
{
    MockLogger mockLogger;
    MockEventQueue mockEventQueue;
    MockEventDispatcher mockEventDispatcher;
    Engine engine(mockLogger, mockEventQueue, mockEventDispatcher);

    {
        ::testing::InSequence seq;
        EXPECT_CALL(mockLogger, log(Level::Info, "Engine step: tick 3"));
        EXPECT_CALL(mockEventDispatcher, dispatch(Event{.name = antwika::engine::events::kTick}));
        EXPECT_CALL(mockEventQueue, empty()).WillOnce(::testing::Return(false));
        EXPECT_CALL(mockEventQueue, pop()).WillOnce(::testing::Return(Event{.name = "mockEvent"}));
        EXPECT_CALL(mockLogger, log(Level::Info, "Process event: mockEvent"));
        EXPECT_CALL(mockEventQueue, empty()).WillOnce(::testing::Return(true));
    }

    engine.step(3);
}

TEST(EngineTest, Step_PropagatesExceptionWhenDispatcherDispatchFails)
{
    MockLogger mockLogger;
    MockEventQueue mockEventQueue;
    MockEventDispatcher mockEventDispatcher;
    Engine engine(mockLogger, mockEventQueue, mockEventDispatcher);

    EXPECT_CALL(mockLogger, log(Level::Info, "Engine step: tick 0"));
    EXPECT_CALL(mockEventDispatcher, dispatch(Event{.name = antwika::engine::events::kTick}))
        .WillOnce(::testing::Throw(std::runtime_error("mockException")));
    EXPECT_CALL(mockEventQueue, empty()).Times(0);

    EXPECT_THROW(engine.step(0), std::runtime_error);
}

TEST(EngineTest, Step_PropagatesExceptionWhenEventQueuePopFails)
{
    MockLogger mockLogger;
    MockEventQueue mockEventQueue;
    MockEventDispatcher mockEventDispatcher;
    Engine engine(mockLogger, mockEventQueue, mockEventDispatcher);

    EXPECT_CALL(mockLogger, log(Level::Info, "Engine step: tick 0"));
    EXPECT_CALL(mockEventDispatcher, dispatch(Event{.name = antwika::engine::events::kTick}));
    EXPECT_CALL(mockEventQueue, empty()).WillOnce(::testing::Return(false));
    EXPECT_CALL(mockEventQueue, pop()).WillOnce(::testing::Throw(std::runtime_error("mockException")));
    EXPECT_THROW(engine.step(0), std::runtime_error);
}
