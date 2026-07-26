#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <antwika/log/mocks/MockLogger.hpp>
#include <antwika/log/Level.hpp>

#include "antwika/engine/Engine.hpp"
#include "antwika/event/mocks/MockEventQueue.hpp"

using antwika::engine::Engine;
using antwika::event::Event;
using antwika::event::mocks::MockEventQueue;
using antwika::log::Level;
using antwika::log::mocks::MockLogger;

TEST(EngineTest, Start_LogsStartupInfo)
{
    MockLogger mockLogger;
    MockEventQueue mockEventQueue;
    Engine engine(mockLogger, mockEventQueue);
    EXPECT_CALL(mockLogger, log(Level::Info, "Antwika engine started!"));
    EXPECT_CALL(mockEventQueue, empty()).WillOnce(::testing::Return(true));
    engine.start();
}

TEST(EngineTest, Start_ProcessesQueuedEvents)
{
    MockLogger mockLogger;
    MockEventQueue mockEventQueue;
    Engine engine(mockLogger, mockEventQueue);

    {
        ::testing::InSequence seq;
        EXPECT_CALL(mockLogger, log(Level::Info, "Antwika engine started!"));
        EXPECT_CALL(mockEventQueue, empty()).WillOnce(::testing::Return(false));
        EXPECT_CALL(mockEventQueue, pop()).WillOnce(::testing::Return(Event{.name = "mockEvent"}));
        EXPECT_CALL(mockLogger, log(Level::Info, "Process event: mockEvent"));
        EXPECT_CALL(mockEventQueue, empty()).WillOnce(::testing::Return(true));
    }

    engine.start();
}

TEST(EngineTest, Start_PropagatesExceptionWhenEventQueuePopFails)
{
    MockLogger mockLogger;
    MockEventQueue mockEventQueue;
    Engine engine(mockLogger, mockEventQueue);

    EXPECT_CALL(mockLogger, log(Level::Info, "Antwika engine started!"));
    EXPECT_CALL(mockEventQueue, empty()).WillOnce(::testing::Return(false));
    EXPECT_CALL(mockEventQueue, pop()).WillOnce(::testing::Throw(std::runtime_error("mockException")));
    EXPECT_THROW(engine.start(), std::runtime_error);
}
