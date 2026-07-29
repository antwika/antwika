#include <gtest/gtest.h>

#include <antwika/engine/Events.hpp>

#include "antwika/game/Events.hpp"
#include "antwika/game/GameStateReducer.hpp"
#include "antwika/game/GameStateReducerError.hpp"

using antwika::event::Event;
using antwika::event::TickEvent;
using antwika::game::GameState;
using antwika::game::GameStateReducer;
using antwika::game::GameStateReducerError;

TEST(GameStateReducerTest, Handle_IncrementsTicksProcessedOnBuiltInTickEvent)
{
    GameState state;
    GameStateReducer reducer(state);

    reducer.handle(TickEvent{
        .tick = 0,
        .event = Event{.name = antwika::engine::events::kTick},
    });
    reducer.handle(TickEvent{
        .tick = 1,
        .event = Event{.name = antwika::engine::events::kTick},
    });

    EXPECT_EQ(state.ticksProcessed, 2);
    EXPECT_EQ(state.score, 0);
}

TEST(GameStateReducerTest, Handle_AddsToScoreOnCustomScoreIncrementEvent)
{
    GameState state;
    GameStateReducer reducer(state);

    reducer.handle(TickEvent{
        .tick = 0,
        .event = Event{
            .name = antwika::game::events::kScoreIncrement,
            .payload = R"({"amount":5})",
        },
    });
    reducer.handle(TickEvent{
        .tick = 1,
        .event = Event{
            .name = antwika::game::events::kScoreIncrement,
            .payload = R"({"amount":3})",
        },
    });

    EXPECT_EQ(state.score, 8);
    EXPECT_EQ(state.ticksProcessed, 0);
}

TEST(GameStateReducerTest, Handle_IgnoresUnrelatedEvents)
{
    GameState state;
    GameStateReducer reducer(state);

    reducer.handle(TickEvent{
        .tick = 0,
        .event = Event{.name = "some.other.event"},
    });

    EXPECT_EQ(state, GameState{});
}

TEST(GameStateReducerTest, Handle_ScoreIncrementPayloadThatIsNotJsonThrows)
{
    GameState state;
    GameStateReducer reducer(state);

    EXPECT_THROW(
        reducer.handle(TickEvent{
            .tick = 0,
            .event = Event{
                .name = antwika::game::events::kScoreIncrement,
                .payload = "abc",
            },
        }),
        GameStateReducerError);
}

TEST(GameStateReducerTest, Handle_ScoreIncrementPayloadWithNegativeAmountThrows)
{
    GameState state;
    GameStateReducer reducer(state);

    EXPECT_THROW(
        reducer.handle(TickEvent{
            .tick = 0,
            .event = Event{
                .name = antwika::game::events::kScoreIncrement,
                .payload = R"({"amount":-1})",
            },
        }),
        GameStateReducerError);
}

TEST(GameStateReducerTest, Handle_ScoreIncrementPayloadMissingAmountThrows)
{
    GameState state;
    GameStateReducer reducer(state);

    EXPECT_THROW(
        reducer.handle(TickEvent{
            .tick = 0,
            .event = Event{
                .name = antwika::game::events::kScoreIncrement,
                .payload = "{}",
            },
        }),
        GameStateReducerError);
}

TEST(
    GameStateReducerTest,
    Handle_ReactsToBuiltInAndCustomEventsThroughTheSameMechanism)
{
    GameState state;
    GameStateReducer reducer(state);

    reducer.handle(TickEvent{
        .tick = 0,
        .event = Event{.name = antwika::engine::events::kTick},
    });
    reducer.handle(TickEvent{
        .tick = 0,
        .event = Event{
            .name = antwika::game::events::kScoreIncrement,
            .payload = R"({"amount":10})",
        },
    });
    reducer.handle(TickEvent{
        .tick = 1,
        .event = Event{.name = antwika::engine::events::kTick},
    });

    EXPECT_EQ(state, (GameState{.ticksProcessed = 2, .score = 10}));
}
