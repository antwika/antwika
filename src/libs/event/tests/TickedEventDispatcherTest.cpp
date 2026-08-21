#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <stdexcept>

#include <antwika/event/mocks/MockEventDispatcher.hpp>
#include <antwika/event/mocks/MockTickEventSink.hpp>

#include "antwika/event/EventError.hpp"
#include "antwika/event/TickedEventDispatcher.hpp"

using antwika::event::Event;
using antwika::event::TickedEventDispatcher;
using antwika::event::TickEvent;
using antwika::event::mocks::MockEventDispatcher;
using antwika::event::mocks::MockTickEventSink;

TEST(TickedEventDispatcherTest, Dispatch_RefusesReentryFromASink)
{
    MockEventDispatcher mockDispatcher;
    MockTickEventSink reentrantSink;
    TickedEventDispatcher tickedEventDispatcher(
        mockDispatcher, {reentrantSink});

    EXPECT_CALL(mockDispatcher, dispatch(::testing::_)).Times(1);
    EXPECT_CALL(reentrantSink, handle(::testing::_))
        .WillOnce(
            [&tickedEventDispatcher](const TickEvent &)
            {
                tickedEventDispatcher.dispatch(
                    Event{.name = "derived"});
            });

    EXPECT_THROW(
        tickedEventDispatcher.dispatch(Event{.name = "cause"}),
        antwika::event::EventError);
}

TEST(TickedEventDispatcherTest, Dispatch_RecoversFromAThrowingSink)
{
    MockEventDispatcher mockDispatcher;
    MockTickEventSink throwingSink;
    TickedEventDispatcher tickedEventDispatcher(
        mockDispatcher, {throwingSink});

    EXPECT_CALL(mockDispatcher, dispatch(::testing::_)).Times(2);
    EXPECT_CALL(throwingSink, handle(::testing::_))
        .WillOnce(
            [](const TickEvent &)
            { throw std::runtime_error("a sink's own failure"); })
        .WillOnce([](const TickEvent &) {});

    EXPECT_THROW(
        tickedEventDispatcher.dispatch(Event{.name = "first"}),
        std::runtime_error);

    tickedEventDispatcher.dispatch(Event{.name = "second"});
}

TEST(
    TickedEventDispatcherTest,
    Dispatch_ForwardsToWrappedDispatcherAndStampsCurrentTick)
{
    MockEventDispatcher mockDispatcher;
    MockTickEventSink mockTickEventSink;
    TickedEventDispatcher tickedEventDispatcher(
        mockDispatcher, {mockTickEventSink});
    Event mockEvent{.name = "mockEvent"};

    tickedEventDispatcher.setTick(5);

    {
        ::testing::InSequence seq;
        EXPECT_CALL(mockDispatcher, dispatch(mockEvent)).Times(1);
        EXPECT_CALL(
            mockTickEventSink,
            handle(TickEvent{.tick = 5, .event = mockEvent}))
            .Times(1);
    }

    tickedEventDispatcher.dispatch(mockEvent);
}

TEST(TickedEventDispatcherTest, Dispatch_NotifiesEveryTimedSink)
{
    MockEventDispatcher mockDispatcher;
    MockTickEventSink firstSink;
    MockTickEventSink secondSink;
    TickedEventDispatcher tickedEventDispatcher(
        mockDispatcher, {firstSink, secondSink});
    Event mockEvent{.name = "mockEvent"};

    tickedEventDispatcher.setTick(3);

    EXPECT_CALL(mockDispatcher, dispatch(mockEvent)).Times(1);
    EXPECT_CALL(firstSink, handle(TickEvent{.tick = 3, .event = mockEvent}))
        .Times(1);
    EXPECT_CALL(secondSink, handle(TickEvent{.tick = 3, .event = mockEvent}))
        .Times(1);

    tickedEventDispatcher.dispatch(mockEvent);
}

TEST(
    TickedEventDispatcherTest,
    Dispatch_StampsAdvancingTicksAcrossMultipleCalls)
{
    MockEventDispatcher mockDispatcher;
    MockTickEventSink mockTickEventSink;
    TickedEventDispatcher tickedEventDispatcher(
        mockDispatcher, {mockTickEventSink});
    Event mockEvent1{.name = "mockEvent1"};
    Event mockEvent2{.name = "mockEvent2"};

    EXPECT_CALL(mockDispatcher, dispatch(::testing::_)).Times(2);

    {
        ::testing::InSequence seq;
        EXPECT_CALL(
            mockTickEventSink,
            handle(TickEvent{.tick = 0, .event = mockEvent1}))
            .Times(1);
        EXPECT_CALL(
            mockTickEventSink,
            handle(TickEvent{.tick = 1, .event = mockEvent2}))
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
    MockTickEventSink mockTickEventSink;
    TickedEventDispatcher tickedEventDispatcher(
        mockDispatcher, {mockTickEventSink});
    Event mockEvent{.name = "mockEvent"};

    EXPECT_CALL(mockDispatcher, dispatch(mockEvent))
        .WillOnce(::testing::Throw(std::runtime_error("mockException")));
    EXPECT_CALL(mockTickEventSink, handle(::testing::_)).Times(0);

    EXPECT_THROW(tickedEventDispatcher.dispatch(mockEvent), std::runtime_error);
}
