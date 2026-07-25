#include <gtest/gtest.h>

#include "antwika/event/EventQueue.hpp"
#include "antwika/event/mocks/MockEventQueue.hpp"
#include "antwika/event/mocks/MockEventRecorder.hpp"

using antwika::event::Event;
using antwika::event::EventQueue;
using antwika::event::EventRecorder;
using antwika::event::mocks::MockEventRecorder;

TEST(EventQueueTest, enqueue_records_event)
{
    MockEventRecorder mockEventRecorder;
    EventQueue eventQueue(mockEventRecorder);
    Event mockEvent{.name = "mockEvent"};
    EXPECT_CALL(mockEventRecorder, record(mockEvent));
    eventQueue.enqueue(mockEvent);
}

TEST(EventQueueTest, enqueue_adds_event_to_queue)
{
    EventRecorder eventRecorder;
    EventQueue eventQueue(eventRecorder);
    Event mockEvent = {.name = "mockEvent"};
    eventQueue.enqueue(mockEvent);
    auto event = eventQueue.pop();
    EXPECT_EQ(event, mockEvent);
}

TEST(EventQueueTest, pop_returns_fifo_order)
{
    EventRecorder eventRecorder;
    EventQueue eventQueue(eventRecorder);
    Event mockEvent1 = {.name = "mockEvent1"};
    Event mockEvent2 = {.name = "mockEvent2"};
    eventQueue.enqueue(mockEvent1);
    eventQueue.enqueue(mockEvent2);
    auto event1 = eventQueue.pop();
    auto event2 = eventQueue.pop();
    EXPECT_EQ(event1, mockEvent1);
    EXPECT_EQ(event2, mockEvent2);
}

TEST(EventQueueTest, empty_returns_true_when_nothing_has_been_enqueued)
{
    EventRecorder eventRecorder;
    EventQueue eventQueue(eventRecorder);
    EXPECT_TRUE(eventQueue.empty());
}

TEST(EventQueueTest, empty_returns_false_after_enqueue)
{
    EventRecorder eventRecorder;
    EventQueue eventQueue(eventRecorder);
    eventQueue.enqueue(Event{.name = "mockEvent"});
    EXPECT_FALSE(eventQueue.empty());
}

TEST(EventQueueTest, getHistory)
{
    MockEventRecorder mockEventRecorder;
    std::vector<Event> mockEvents = std::vector<Event>{Event{.name = "mockEvent"}};
    EXPECT_CALL(mockEventRecorder, getEvents()).WillOnce(::testing::Return(mockEvents));
    EventQueue eventQueue(mockEventRecorder);
    auto history = eventQueue.getHistory();
    EXPECT_EQ(history, mockEvents);
}
