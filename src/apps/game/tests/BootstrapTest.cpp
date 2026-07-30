#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <vector>

#include <antwika/ecs/ISystem.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/engine/Events.hpp>
#include <antwika/event/Event.hpp>
#include <antwika/event/EventRecorder.hpp>
#include <antwika/event/TickEvent.hpp>
#include <antwika/event/TickEventRecorder.hpp>
#include <antwika/input/InputEvent.hpp>
#include <antwika/input/InputEventCodec.hpp>
#include <antwika/input/MouseButton.hpp>
#include <antwika/log/mocks/MockLogger.hpp>
#include <antwika/replay/EngineLoopError.hpp>
#include <antwika/replay/ReplaySource.hpp>

#include "antwika/game/Camera.hpp"
#include "antwika/game/Events.hpp"
#include "antwika/game/Game.hpp"
#include "antwika/game/Cell.hpp"
#include "antwika/game/Direction.hpp"
#include "antwika/game/GridExtent.hpp"
#include "antwika/game/IsoProjection.hpp"
#include "antwika/game/PathIndex.hpp"

using antwika::event::Event;
using antwika::event::EventRecorder;
using antwika::event::TickEvent;
using antwika::event::TickEventRecorder;
using antwika::game::Camera;
using antwika::game::GameState;
using antwika::game::GridExtent;
using antwika::game::PathIndex;
using antwika::input::InputEventCodec;
using antwika::log::mocks::MockLogger;
using antwika::replay::EngineLoopError;
using antwika::replay::ReplaySource;
using ::testing::NiceMock;

namespace
{
    constexpr GridExtent kExtent{.width = 16, .height = 16};

    // Bootstrap wires together already-unit-tested collaborators.
    // Re-testing their call sequences here would be redundant.
    // It would also be brittle to maintain over time.
    // These tests check the wiring end to end, black-box style.
    struct Harness
    {
        NiceMock<MockLogger> logger;
        EventRecorder eventSink;
        InputEventCodec codec;
        Camera camera;
        PathIndex paths;

        antwika::game::GameSummary run(
            ReplaySource &source,
            antwika::time::Tick maxTicks,
            antwika::event::ITickEventSink *recorder = nullptr)
        {
            return antwika::game::bootstrap(
                logger,
                eventSink,
                source,
                codec,
                kExtent,
                camera,
                paths,
                {},
                maxTicks,
                recorder);
        }
    };
} // namespace

TEST(BootstrapTest, Bootstrap_RunsScriptedTicksAndReturnsTheGameState)
{
    Harness harness;
    ReplaySource source({
        TickEvent{
            .tick = 1,
            .event = Event{
                .name = antwika::game::events::kScoreIncrement,
                .payload = R"({"amount":5})",
            },
        },
        TickEvent{
            .tick = 3,
            .event = Event{
                .name = antwika::game::events::kScoreIncrement,
                .payload = R"({"amount":2})",
            },
        },
        TickEvent{
            .tick = 4,
            .event = Event{.name = antwika::engine::events::kStop},
        },
    });

    const auto summary = harness.run(source, 10);

    EXPECT_EQ(
        summary.state, (GameState{.ticksProcessed = 5, .score = 7}));
}

TEST(BootstrapTest, Bootstrap_WithNoScriptedInputOnlyAdvancesTicks)
{
    Harness harness;
    ReplaySource source({
        TickEvent{
            .tick = 2,
            .event = Event{.name = antwika::engine::events::kStop},
        },
    });

    const auto summary = harness.run(source, 10);

    EXPECT_EQ(
        summary.state, (GameState{.ticksProcessed = 3, .score = 0}));
    EXPECT_TRUE(summary.paths.empty());
    EXPECT_TRUE(summary.walkers.empty());
}

// A caller wanting to persist a `--record` file has no pre-known script.
// It instead passes an optional replayRecorder.
// bootstrap() must register it so it observes every dispatched event.
TEST(BootstrapTest, Bootstrap_ForwardsDispatchedEventsToATickEventRecorder)
{
    Harness harness;
    ReplaySource source({
        TickEvent{
            .tick = 0,
            .event = Event{
                .name = antwika::game::events::kScoreIncrement,
                .payload = R"({"amount":5})",
            },
        },
        TickEvent{
            .tick = 0,
            .event = Event{.name = antwika::engine::events::kStop},
        },
    });
    TickEventRecorder recorder;

    harness.run(source, 10, &recorder);

    EXPECT_EQ(
        recorder.getEvents(),
        (std::vector<TickEvent>{
            TickEvent{
                .tick = 0,
                .event = Event{
                    .name = antwika::game::events::kScoreIncrement,
                    .payload = R"({"amount":5})",
                },
            },
            TickEvent{
                .tick = 0,
                .event = Event{.name = antwika::engine::events::kStop},
            },
            TickEvent{
                .tick = 0,
                .event = Event{.name = antwika::engine::events::kTick},
            },
        }));
}

// Safety valve: a run that never dispatches engine.stop must fail loudly.
// It should not hang or silently truncate once maxTicks is reached.
TEST(BootstrapTest, Bootstrap_ThrowsWhenMaxTicksIsReachedWithoutAStopEvent)
{
    Harness harness;
    ReplaySource source({});

    EXPECT_THROW(harness.run(source, 3), EngineLoopError);
}

// The whole feature, through the front door.
// Clicks lay a path, a click drops a walker, and the walker moves.
TEST(BootstrapTest, Bootstrap_LaysAPathDropsAWalkerAndWalksIt)
{
    Harness harness;
    const InputEventCodec codec;

    const auto pressAt = [&codec](antwika::game::Cell cell,
                                  antwika::input::MouseButton button)
    {
        const auto point =
            antwika::game::cellCentre(cell, antwika::game::Camera());
        return codec.encode(
            antwika::input::PointerButtonPressed{
                .button = button,
                .position = {.x = point.x, .y = point.y}});
    };

    const std::vector<TickEvent> script{
        TickEvent{
            .tick = 0,
            .event = pressAt(
                antwika::game::Cell{.x = 1, .y = 1},
                antwika::input::MouseButton::Left)},
        TickEvent{
            .tick = 0,
            .event = pressAt(
                antwika::game::Cell{.x = 2, .y = 1},
                antwika::input::MouseButton::Left)},
        TickEvent{
            .tick = 0,
            .event = pressAt(
                antwika::game::Cell{.x = 1, .y = 1},
                antwika::input::MouseButton::Right)},
        // Stopping on the same tick keeps this to exactly one step.
        // The two-cell path is a dead end at both ends.
        // A longer run would oscillate and say nothing about the move.
        TickEvent{
            .tick = 0,
            .event = Event{.name = antwika::engine::events::kStop}},
    };
    ReplaySource source(script);

    const auto summary = harness.run(source, 10);

    EXPECT_EQ(summary.paths.size(), 2U);
    ASSERT_EQ(summary.walkers.size(), 1U);

    // Dropped on (1,1) facing east, with (2,1) the only way on.
    EXPECT_EQ(summary.walkers[0].at, (antwika::game::Cell{.x = 2, .y = 1}));
    EXPECT_EQ(summary.walkers[0].facing, antwika::game::Direction::East);
}

namespace
{
    // Counts the ticks it observes, and touches nothing.
    class CountingObserver final : public antwika::ecs::ISystem
    {
    public:
        void update(antwika::ecs::World &, antwika::time::Tick) override
        {
            ++ticks;
        }

        std::size_t ticks = 0;
    };
} // namespace

// Observers run in a phase of their own, after the walk.
// A renderer registered here sees the generation this tick produced.
TEST(BootstrapTest, Bootstrap_RunsEveryObserverOncePerTick)
{
    Harness harness;
    ReplaySource source({
        TickEvent{
            .tick = 2,
            .event = Event{.name = antwika::engine::events::kStop},
        },
    });

    CountingObserver first;
    CountingObserver second;

    antwika::game::bootstrap(
        harness.logger,
        harness.eventSink,
        source,
        harness.codec,
        kExtent,
        harness.camera,
        harness.paths,
        {first, second},
        10);

    // Ticks 0, 1 and 2 all run, and the stop ends it after the third.
    EXPECT_EQ(first.ticks, 3U);
    EXPECT_EQ(second.ticks, 3U);
}
