#include <gtest/gtest.h>

#include <antwika/event/mocks/MockEventSink.hpp>

#include "antwika/event/EventDispatcher.hpp"

using antwika::event::Event;
using antwika::event::EventDispatcher;
using antwika::event::mocks::MockEventSink;

TEST(EventDispatcherTest, Dispatch_DeliversTheEventToEverySink)
{
    MockEventSink mockEventSink;
    EventDispatcher eventDispatcher({mockEventSink});
    Event mockEvent{.name = "mockEvent"};

    EXPECT_CALL(mockEventSink, handle(mockEvent)).Times(1);

    eventDispatcher.dispatch(mockEvent);
}

TEST(EventDispatcherTest, Dispatch_PropagatesExceptionWhenASinkHandleFails)
{
    MockEventSink mockEventSink;
    EventDispatcher eventDispatcher({mockEventSink});
    Event mockEvent{.name = "mockEvent"};

    EXPECT_CALL(mockEventSink, handle(mockEvent))
        .WillOnce(::testing::Throw(std::runtime_error("mockException")));

    EXPECT_THROW(eventDispatcher.dispatch(mockEvent), std::runtime_error);
}
