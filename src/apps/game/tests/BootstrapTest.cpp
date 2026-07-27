#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <antwika/event/mocks/MockEventQueue.hpp>
#include <antwika/event/mocks/MockEventSink.hpp>
#include <antwika/log/mocks/MockAppender.hpp>
#include <antwika/log/mocks/MockFormatter.hpp>
#include <antwika/log/mocks/MockLogPolicy.hpp>
#include <antwika/time/fakes/FakeClock.hpp>

#include "antwika/game/Game.hpp"

using antwika::event::Event;
using antwika::event::mocks::MockEventQueue;
using antwika::event::mocks::MockEventSink;
using antwika::log::Level;
using antwika::log::mocks::MockAppender;
using antwika::log::mocks::MockFormatter;
using antwika::log::mocks::MockLogPolicy;
using antwika::time::fakes::FakeClock;

TEST(BootstrapTest, Bootstrap_WiresCollaboratorsAndProcessesDispatchedEvent)
{
    std::chrono::system_clock::time_point time{};
    FakeClock fakeClock(time);
    MockAppender mockAppender;
    MockFormatter mockFormatter;
    MockLogPolicy mockLogPolicy;
    MockEventQueue mockEventQueue;
    MockEventSink mockEventSink;

    const Event runningEvent{.name = "Running Antwika Game"};

    {
        ::testing::InSequence seq;

        EXPECT_CALL(mockEventSink, handle(runningEvent));
        EXPECT_CALL(mockEventQueue, enqueue(runningEvent));

        EXPECT_CALL(mockLogPolicy, accepts(Level::Info)).WillOnce(::testing::Return(true));
        EXPECT_CALL(mockFormatter, format(time, Level::Info, "Antwika engine started!"))
            .WillOnce(::testing::Return("Formatted: Antwika engine started!"));
        EXPECT_CALL(mockAppender, append("Formatted: Antwika engine started!"));

        EXPECT_CALL(mockEventQueue, empty()).WillOnce(::testing::Return(false));
        EXPECT_CALL(mockEventQueue, pop()).WillOnce(::testing::Return(runningEvent));

        EXPECT_CALL(mockLogPolicy, accepts(Level::Info)).WillOnce(::testing::Return(true));
        EXPECT_CALL(mockFormatter, format(time, Level::Info, "Process event: Running Antwika Game"))
            .WillOnce(::testing::Return("Formatted: Process event: Running Antwika Game"));
        EXPECT_CALL(mockAppender, append("Formatted: Process event: Running Antwika Game"));

        EXPECT_CALL(mockEventQueue, empty()).WillOnce(::testing::Return(true));
    }

    antwika::game::bootstrap(fakeClock, mockAppender, mockFormatter, mockLogPolicy, mockEventQueue, mockEventSink);
}
