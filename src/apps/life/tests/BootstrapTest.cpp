#include <gtest/gtest.h>

#include <sstream>
#include <vector>

#include <antwika/ecs/ISystem.hpp>
#include <antwika/event/Event.hpp>
#include <antwika/event/EventQueue.hpp>
#include <antwika/event/EventRecorder.hpp>
#include <antwika/event/TimedEvent.hpp>
#include <antwika/log/Level.hpp>
#include <antwika/log/MinimumLevelLogPolicy.hpp>
#include <antwika/log/NullAppender.hpp>
#include <antwika/log/PlainFormatter.hpp>
#include <antwika/replay/ReplaySource.hpp>
#include <antwika/time/fakes/FakeClock.hpp>

#include "antwika/life/Events.hpp"
#include "antwika/life/Life.hpp"
#include "antwika/life/PrintSystem.hpp"

using antwika::ecs::ISystem;
using antwika::ecs::World;
using antwika::event::Event;
using antwika::event::EventQueue;
using antwika::event::EventRecorder;
using antwika::event::TimedEvent;
using antwika::life::Board;
using antwika::life::PrintSystem;
using antwika::log::Level;
using antwika::log::MinimumLevelLogPolicy;
using antwika::log::NullAppender;
using antwika::log::PlainFormatter;
using antwika::replay::ReplaySource;
using antwika::time::fakes::FakeClock;

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
    std::chrono::system_clock::time_point time{};
    FakeClock fakeClock(time);
    NullAppender appender;
    PlainFormatter formatter;
    MinimumLevelLogPolicy logPolicy(Level::Info);
    EventQueue eventQueue;
    EventRecorder eventSink;

    ReplaySource inputSource({
        TimedEvent{
            .tick = 0,
            .event = Event{
                .name = antwika::life::events::kToggleCell,
                .payload = "1,2",
            },
        },
        TimedEvent{
            .tick = 0,
            .event = Event{
                .name = antwika::life::events::kToggleCell,
                .payload = "2,2",
            },
        },
        TimedEvent{
            .tick = 0,
            .event = Event{
                .name = antwika::life::events::kToggleCell,
                .payload = "3,2",
            },
        },
    });

    auto board = antwika::life::bootstrap(
        fakeClock,
        appender,
        formatter,
        logPolicy,
        eventQueue,
        eventSink,
        inputSource,
        4,
        5,
        5);

    EXPECT_EQ(board.width, 5U);
    EXPECT_EQ(board.height, 5U);
    EXPECT_EQ(board.alive, horizontalBlinkerOn5x5());
}

// This is the requirement this feature exists for.
// Independent observer systems can watch every generation, not just the last.
// Registering more than one doesn't make them interfere with each other.
TEST(BootstrapTest, Bootstrap_RunsEveryObserverOncePerTick)
{
    std::chrono::system_clock::time_point time{};
    FakeClock fakeClock(time);
    NullAppender appender;
    PlainFormatter formatter;
    MinimumLevelLogPolicy logPolicy(Level::Info);
    EventQueue eventQueue;
    EventRecorder eventSink;

    ReplaySource inputSource({
        TimedEvent{
            .tick = 0,
            .event = Event{
                .name = antwika::life::events::kToggleCell,
                .payload = "1,2",
            },
        },
        TimedEvent{
            .tick = 0,
            .event = Event{
                .name = antwika::life::events::kToggleCell,
                .payload = "2,2",
            },
        },
        TimedEvent{
            .tick = 0,
            .event = Event{
                .name = antwika::life::events::kToggleCell,
                .payload = "3,2",
            },
        },
    });

    std::ostringstream printed;
    PrintSystem printSystem(5, printed);
    CallCountingSystem countingSystem;

    auto board = antwika::life::bootstrap(
        fakeClock,
        appender,
        formatter,
        logPolicy,
        eventQueue,
        eventSink,
        inputSource,
        4,
        5,
        5,
        {printSystem, countingSystem});

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
    std::chrono::system_clock::time_point time{};
    FakeClock fakeClock(time);
    NullAppender appender;
    PlainFormatter formatter;
    MinimumLevelLogPolicy logPolicy(Level::Info);
    EventQueue eventQueue;
    EventRecorder eventSink;

    ReplaySource inputSource({});

    auto board = antwika::life::bootstrap(
        fakeClock,
        appender,
        formatter,
        logPolicy,
        eventQueue,
        eventSink,
        inputSource,
        3,
        4,
        4);

    EXPECT_EQ(board.alive, std::vector<bool>(16, false));
}
