#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <stdexcept>

#include <antwika/event/mocks/MockEventSink.hpp>

#include "antwika/event/EventDispatcher.hpp"

using antwika::event::Event;
using antwika::event::EventName;
using antwika::event::EventDispatcher;
using antwika::event::mocks::MockEventSink;

TEST(EventDispatcherTest, Dispatch_DeliversTheEventToEverySink)
{
    MockEventSink firstSink;
    MockEventSink secondSink;
    EventDispatcher eventDispatcher({firstSink, secondSink});
    Event mockEvent{.name = EventName{"mockEvent"}};

    EXPECT_CALL(firstSink, handle(mockEvent)).Times(1);
    EXPECT_CALL(secondSink, handle(mockEvent)).Times(1);

    eventDispatcher.dispatch(mockEvent);
}

TEST(EventDispatcherTest, Dispatch_DeliversInTheOrderTheSinksWereGiven)
{
    MockEventSink firstSink;
    MockEventSink secondSink;
    EventDispatcher eventDispatcher({firstSink, secondSink});
    Event mockEvent{.name = EventName{"mockEvent"}};

    {
        ::testing::InSequence seq;
        EXPECT_CALL(firstSink, handle(mockEvent)).Times(1);
        EXPECT_CALL(secondSink, handle(mockEvent)).Times(1);
    }

    eventDispatcher.dispatch(mockEvent);
}

TEST(EventDispatcherTest, Dispatch_StopsAtTheSinkThatThrew)
{
    MockEventSink throwingSink;
    MockEventSink laterSink;
    EventDispatcher eventDispatcher({throwingSink, laterSink});
    Event mockEvent{.name = EventName{"mockEvent"}};

    EXPECT_CALL(throwingSink, handle(mockEvent))
        .WillOnce(::testing::Throw(std::runtime_error("mockException")));
    EXPECT_CALL(laterSink, handle(::testing::_)).Times(0);

    EXPECT_THROW(eventDispatcher.dispatch(mockEvent), std::runtime_error);
}

TEST(EventDispatcherTest, Dispatch_PropagatesExceptionWhenASinkHandleFails)
{
    MockEventSink mockEventSink;
    EventDispatcher eventDispatcher({mockEventSink});
    Event mockEvent{.name = EventName{"mockEvent"}};

    EXPECT_CALL(mockEventSink, handle(mockEvent))
        .WillOnce(::testing::Throw(std::runtime_error("mockException")));

    EXPECT_THROW(eventDispatcher.dispatch(mockEvent), std::runtime_error);
}
