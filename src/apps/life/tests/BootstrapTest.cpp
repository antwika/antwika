#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <sstream>
#include <vector>

#include <antwika/ecs/ISystem.hpp>
#include <antwika/engine/Events.hpp>
#include <antwika/event/Event.hpp>
#include <antwika/event/EventRecorder.hpp>
#include <antwika/event/TickEventRecorder.hpp>
#include <antwika/event/TickEvent.hpp>
#include <antwika/log/mocks/MockLogger.hpp>
#include <antwika/replay/EngineLoopError.hpp>
#include <antwika/replay/ReplaySource.hpp>

#include "antwika/life/Events.hpp"
#include "antwika/life/Life.hpp"
#include "antwika/life/PrintSystem.hpp"

using antwika::ecs::ISystem;
using antwika::ecs::World;
using antwika::event::Event;
using antwika::event::EventRecorder;
using antwika::event::TickEventRecorder;
using antwika::event::TickEvent;
using antwika::life::Board;
using antwika::life::PrintSystem;
using antwika::log::mocks::MockLogger;
using antwika::replay::EngineLoopError;
using antwika::replay::ReplaySource;
using ::testing::NiceMock;

namespace
{
    // A period-2 blinker, seeded horizontally.
    // Back to its start after an even number of generations.
    std::vector<bool> horizontalBlinkerOn5x5()
    {
        std::vector<bool> alive(25, false);
        alive[2 * 5 + 1] = true;
        alive[2 * 5 + 2] = true;
        alive[2 * 5 + 3] = true;
        return alive;
    }

    // A second, independent observer standing in for a future stats collector.
    // It knows nothing about PrintSystem.
    // Registering both together proves neither interferes with the other.
    class CallCountingSystem final : public ISystem
    {
    public:
        void update(World &, antwika::time::Tick) override
        {
            ++calls;
        }

        int calls = 0;
    };
} // namespace

// Bootstrap wires together many already-unit-tested collaborators.
// Re-testing their exact call sequences here would be redundant.
// It would also be brittle to maintain over time.
// This test instead verifies the wiring end to end, black-box style.
// It seeds a blinker via scripted toggle events, then checks the board.
TEST(BootstrapTest, Bootstrap_RunsScriptedTicksAndReturnsResultingBoard)
{
    NiceMock<MockLogger> logger;
    EventRecorder eventSink;

    ReplaySource inputSource({
        TickEvent{
            .tick = 0,
            .event = Event{
                .name = antwika::life::events::kToggleCell,
                .payload = R"({"x":1,"y":2})",
            },
        },
        TickEvent{
            .tick = 0,
            .event = Event{
                .name = antwika::life::events::kToggleCell,
                .payload = R"({"x":2,"y":2})",
            },
        },
        TickEvent{
            .tick = 0,
            .event = Event{
                .name = antwika::life::events::kToggleCell,
                .payload = R"({"x":3,"y":2})",
            },
        },
        TickEvent{
            .tick = 3,
            .event = Event{.name = antwika::engine::events::kStop},
        },
    });

    auto board = antwika::life::bootstrap(
        logger,
        eventSink,
        inputSource,
        5,
        5,
        {},
        10);

    EXPECT_EQ(board.width, 5U);
    EXPECT_EQ(board.height, 5U);
    EXPECT_EQ(board.alive, horizontalBlinkerOn5x5());
}

// This is the requirement this feature exists for.
// Independent observer systems can watch every generation, not just the last.
// Registering more than one doesn't make them interfere with each other.
TEST(BootstrapTest, Bootstrap_RunsEveryObserverOncePerTick)
{
    NiceMock<MockLogger> logger;
    EventRecorder eventSink;

    ReplaySource inputSource({
        TickEvent{
            .tick = 0,
            .event = Event{
                .name = antwika::life::events::kToggleCell,
                .payload = R"({"x":1,"y":2})",
            },
        },
        TickEvent{
            .tick = 0,
            .event = Event{
                .name = antwika::life::events::kToggleCell,
                .payload = R"({"x":2,"y":2})",
            },
        },
        TickEvent{
            .tick = 0,
            .event = Event{
                .name = antwika::life::events::kToggleCell,
                .payload = R"({"x":3,"y":2})",
            },
        },
        TickEvent{
            .tick = 3,
            .event = Event{.name = antwika::engine::events::kStop},
        },
    });

    std::ostringstream printed;
    PrintSystem printSystem(5, printed);
    CallCountingSystem countingSystem;

    auto board = antwika::life::bootstrap(
        logger,
        eventSink,
        inputSource,
        5,
        5,
        {printSystem, countingSystem},
        10);

    EXPECT_EQ(countingSystem.calls, 4);

    std::ostringstream expected;
    expected << "After tick 0:\n"
             << ".....\n..#..\n..#..\n..#..\n.....\n"
             << "After tick 1:\n"
             << ".....\n.....\n.###.\n.....\n.....\n"
             << "After tick 2:\n"
             << ".....\n..#..\n..#..\n..#..\n.....\n"
             << "After tick 3:\n"
             << ".....\n.....\n.###.\n.....\n.....\n";
    EXPECT_EQ(printed.str(), expected.str());
    EXPECT_EQ(board.alive, horizontalBlinkerOn5x5());
}

TEST(BootstrapTest, Bootstrap_WithNoScriptedInputStaysAllDead)
{
    NiceMock<MockLogger> logger;
    EventRecorder eventSink;

    ReplaySource inputSource({
        TickEvent{
            .tick = 2,
            .event = Event{.name = antwika::engine::events::kStop},
        },
    });

    auto board = antwika::life::bootstrap(
        logger,
        eventSink,
        inputSource,
        4,
        4,
        {},
        10);

    EXPECT_EQ(board.alive, std::vector<bool>(16, false));
}

// A caller wanting to persist a `--record` file has no pre-known script.
// It instead passes an optional replayRecorder.
// bootstrap() must register it so it observes every dispatched event.
TEST(BootstrapTest, Bootstrap_ForwardsDispatchedEventsToATickEventRecorder)
{
    NiceMock<MockLogger> logger;
    EventRecorder eventSink;

    ReplaySource inputSource({
        TickEvent{
            .tick = 0,
            .event = Event{
                .name = antwika::life::events::kToggleCell,
                .payload = R"({"x":1,"y":2})",
            },
        },
        TickEvent{
            .tick = 0,
            .event = Event{.name = antwika::engine::events::kStop},
        },
    });
    TickEventRecorder replayRecorder;

    antwika::life::bootstrap(
        logger,
        eventSink,
        inputSource,
        4,
        4,
        {},
        10,
        &replayRecorder);

    EXPECT_EQ(
        replayRecorder.getEvents(),
        (std::vector<TickEvent>{
            TickEvent{
                .tick = 0,
                .event = Event{
                    .name = antwika::life::events::kToggleCell,
                    .payload = R"({"x":1,"y":2})",
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
    NiceMock<MockLogger> logger;
    EventRecorder eventSink;

    ReplaySource inputSource({});

    EXPECT_THROW(
        antwika::life::bootstrap(
            logger,
            eventSink,
            inputSource,
            4,
            4,
            {},
            3),
        EngineLoopError);
}
