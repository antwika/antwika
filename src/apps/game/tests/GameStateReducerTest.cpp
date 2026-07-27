#include <gtest/gtest.h>

#include <antwika/engine/Events.hpp>

#include "antwika/game/Events.hpp"
#include "antwika/game/GameStateReducer.hpp"

using antwika::event::Event;
using antwika::event::TimedEvent;
using antwika::game::GameState;
using antwika::game::GameStateReducer;

TEST(GameStateReducerTest, Handle_IncrementsTicksProcessedOnBuiltInTickEvent)
{
    GameState state;
    GameStateReducer reducer(state);

    reducer.handle(TimedEvent{.tick = 0, .event = Event{.name = antwika::engine::events::kTick}});
    reducer.handle(TimedEvent{.tick = 1, .event = Event{.name = antwika::engine::events::kTick}});

    EXPECT_EQ(state.ticksProcessed, 2);
    EXPECT_EQ(state.score, 0);
}

TEST(GameStateReducerTest, Handle_AddsToScoreOnCustomScoreIncrementEvent)
{
    GameState state;
    GameStateReducer reducer(state);

    reducer.handle(TimedEvent{.tick = 0, .event = Event{.name = antwika::game::events::kScoreIncrement, .payload = "5"}});
    reducer.handle(TimedEvent{.tick = 1, .event = Event{.name = antwika::game::events::kScoreIncrement, .payload = "3"}});

    EXPECT_EQ(state.score, 8);
    EXPECT_EQ(state.ticksProcessed, 0);
}

TEST(GameStateReducerTest, Handle_IgnoresUnrelatedEvents)
{
    GameState state;
    GameStateReducer reducer(state);

    reducer.handle(TimedEvent{.tick = 0, .event = Event{.name = "some.other.event"}});

    EXPECT_EQ(state, GameState{});
}

TEST(GameStateReducerTest, Handle_ReactsToBuiltInAndCustomEventsThroughTheSameMechanism)
{
    GameState state;
    GameStateReducer reducer(state);

    reducer.handle(TimedEvent{.tick = 0, .event = Event{.name = antwika::engine::events::kTick}});
    reducer.handle(TimedEvent{.tick = 0, .event = Event{.name = antwika::game::events::kScoreIncrement, .payload = "10"}});
    reducer.handle(TimedEvent{.tick = 1, .event = Event{.name = antwika::engine::events::kTick}});

    EXPECT_EQ(state, (GameState{.ticksProcessed = 2, .score = 10}));
}
