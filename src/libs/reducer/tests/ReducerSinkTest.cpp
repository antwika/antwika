#include "antwika/reducer/ReducerSink.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <antwika/event/Event.hpp>
#include <antwika/reducer/mocks/MockReducer.hpp>

using antwika::event::Event;
using antwika::event::TickEvent;
using antwika::reducer::ReducerSink;
using antwika::reducer::mocks::MockReducer;
using ::testing::Return;

TEST(ReducerSinkTest, HandleCallsReduceAndStoresTheResult)
{
    MockReducer<int> reducer;
    int state = 1;
    ReducerSink<int> sink(state, reducer);
    const TickEvent event{.tick = 5, .event = Event{.name = "x"}};

    EXPECT_CALL(reducer, reduce(1, event)).WillOnce(Return(42));

    sink.handle(event);

    EXPECT_EQ(state, 42);
}

TEST(ReducerSinkTest, EachHandleCallReducesFromTheLatestState)
{
    MockReducer<int> reducer;
    int state = 0;
    ReducerSink<int> sink(state, reducer);
    const TickEvent event{};

    EXPECT_CALL(reducer, reduce(0, event)).WillOnce(Return(1));
    sink.handle(event);

    EXPECT_CALL(reducer, reduce(1, event)).WillOnce(Return(2));
    sink.handle(event);

    EXPECT_EQ(state, 2);
}
