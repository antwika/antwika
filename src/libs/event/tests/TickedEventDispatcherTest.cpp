#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <antwika/event/mocks/MockEventDispatcher.hpp>
#include <antwika/event/mocks/MockTimedEventSink.hpp>

#include "antwika/event/TickedEventDispatcher.hpp"

using antwika::event::Event;
using antwika::event::TickedEventDispatcher;
using antwika::event::TimedEvent;
using antwika::event::mocks::MockEventDispatcher;
using antwika::event::mocks::MockTimedEventSink;

TEST(
    TickedEventDispatcherTest,
    Dispatch_ForwardsToWrappedDispatcherAndStampsCurrentTick)
{
    MockEventDispatcher mockDispatcher;
    MockTimedEventSink mockTimedEventSink;
    TickedEventDispatcher tickedEventDispatcher(
        mockDispatcher, {mockTimedEventSink});
    Event mockEvent{.name = "mockEvent"};

    tickedEventDispatcher.setTick(5);

    {
        ::testing::InSequence seq;
        EXPECT_CALL(mockDispatcher, dispatch(mockEvent)).Times(1);
        EXPECT_CALL(
            mockTimedEventSink,
            handle(TimedEvent{.tick = 5, .event = mockEvent}))
            .Times(1);
    }

    tickedEventDispatcher.dispatch(mockEvent);
}

TEST(
    TickedEventDispatcherTest,
    Dispatch_StampsAdvancingTicksAcrossMultipleCalls)
{
    MockEventDispatcher mockDispatcher;
    MockTimedEventSink mockTimedEventSink;
    TickedEventDispatcher tickedEventDispatcher(
        mockDispatcher, {mockTimedEventSink});
    Event mockEvent1{.name = "mockEvent1"};
    Event mockEvent2{.name = "mockEvent2"};

    EXPECT_CALL(mockDispatcher, dispatch(::testing::_)).Times(2);

    {
        ::testing::InSequence seq;
        EXPECT_CALL(
            mockTimedEventSink,
            handle(TimedEvent{.tick = 0, .event = mockEvent1}))
            .Times(1);
        EXPECT_CALL(
            mockTimedEventSink,
            handle(TimedEvent{.tick = 1, .event = mockEvent2}))
            .Times(1);
    }

    tickedEventDispatcher.dispatch(mockEvent1);
    tickedEventDispatcher.setTick(1);
    tickedEventDispatcher.dispatch(mockEvent2);
}

TEST(
    TickedEventDispatcherTest,
    Dispatch_PropagatesExceptionFromWrappedDispatcherWithoutNotifyingTimedSinks)
{
    MockEventDispatcher mockDispatcher;
    MockTimedEventSink mockTimedEventSink;
    TickedEventDispatcher tickedEventDispatcher(
        mockDispatcher, {mockTimedEventSink});
    Event mockEvent{.name = "mockEvent"};

    EXPECT_CALL(mockDispatcher, dispatch(mockEvent))
        .WillOnce(::testing::Throw(std::runtime_error("mockException")));
    EXPECT_CALL(mockTimedEventSink, handle(::testing::_)).Times(0);

    EXPECT_THROW(tickedEventDispatcher.dispatch(mockEvent), std::runtime_error);
}
