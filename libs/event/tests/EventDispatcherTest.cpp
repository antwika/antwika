#include <gtest/gtest.h>

#include "antwika/event/EventDispatcher.hpp"
#include "antwika/event/mocks/MockEventQueue.hpp"
#include "antwika/event/mocks/MockEventSink.hpp"

using antwika::event::Event;
using antwika::event::EventDispatcher;
using antwika::event::mocks::MockEventQueue;
using antwika::event::mocks::MockEventSink;

TEST(EventDispatcherTest, dispatch)
{
    MockEventQueue mockEventQueue;
    MockEventSink mockEventSink;
    EventDispatcher eventDispatcher(mockEventQueue, {mockEventSink});
    Event mockEvent{.name = "mockEvent"};

    EXPECT_CALL(mockEventQueue, enqueue(mockEvent)).Times(1);
    EXPECT_CALL(mockEventSink, handle(mockEvent)).Times(1);

    eventDispatcher.dispatch(mockEvent);
}
