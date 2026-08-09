#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <sstream>
#include <vector>

#include <antwika/ecs/ISystem.hpp>
#include <antwika/ecs/mocks/MockSystem.hpp>
#include <antwika/engine/Events.hpp>
#include <antwika/event/Event.hpp>
#include <antwika/event/mocks/MockEventSink.hpp>
#include <antwika/event/TickEventRecorder.hpp>
#include <antwika/event/TickEvent.hpp>
#include <antwika/log/mocks/MockLogger.hpp>
#include <antwika/simulation/EngineLoopError.hpp>
#include <antwika/replay/ReplaySource.hpp>

#include "BlinkerScript.hpp"
#include "antwika/life/Events.hpp"
#include "antwika/life/Life.hpp"
#include "antwika/life/PrintSystem.hpp"

using antwika::ecs::ISystem;
using antwika::ecs::World;
using antwika::ecs::mocks::MockSystem;
using antwika::event::Event;
using antwika::event::mocks::MockEventSink;
using antwika::event::TickEventRecorder;
using antwika::event::TickEvent;
using antwika::life::Board;
using antwika::life::PrintSystem;
using antwika::log::mocks::MockLogger;
using antwika::simulation::EngineLoopError;
using antwika::replay::ReplaySource;
using antwika::life::tests::blinkerScript;
using ::testing::_;
using ::testing::NiceMock;

namespace
{
    std::vector<bool> horizontalBlinkerOn5x5()
    {
        std::vector<bool> alive(25, false);
        alive[2 * 5 + 1] = true;
        alive[2 * 5 + 2] = true;
        alive[2 * 5 + 3] = true;
        return alive;
    }

}

TEST(BootstrapTest, Bootstrap_RunsScriptedTicksAndReturnsResultingBoard)
{
    NiceMock<MockLogger> logger;
    NiceMock<MockEventSink> eventSink;

    ReplaySource inputSource(blinkerScript());

    const auto summary = antwika::life::bootstrap(
        antwika::life::LifeWiring{
            .logger = logger,
            .eventSink = eventSink,
            .inputSource = inputSource,
            .width = 5,
            .height = 5,
            .maxTicks = 10});

    EXPECT_EQ(summary.board.width, 5U);
    EXPECT_EQ(summary.board.height, 5U);
    EXPECT_EQ(summary.board.alive, horizontalBlinkerOn5x5());

    EXPECT_TRUE(summary.console.empty());
}

TEST(BootstrapTest, Bootstrap_RunsEveryObserverOncePerTick)
{
    NiceMock<MockLogger> logger;
    NiceMock<MockEventSink> eventSink;

    ReplaySource inputSource(blinkerScript());

    std::ostringstream printed;
    PrintSystem printSystem(5, printed);
    NiceMock<MockSystem> countingSystem;
    EXPECT_CALL(countingSystem, update(_, _)).Times(4);

    const auto board = antwika::life::bootstrap(
        antwika::life::LifeWiring{
            .logger = logger,
            .eventSink = eventSink,
            .inputSource = inputSource,
            .width = 5,
            .height = 5,
            .observers = {printSystem, countingSystem},
            .maxTicks = 10}).board;

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
    NiceMock<MockEventSink> eventSink;

    ReplaySource inputSource({
        TickEvent{
            .tick = 2,
            .event = Event{.name = antwika::engine::events::kStop},
        },
    });

    const auto board = antwika::life::bootstrap(
        antwika::life::LifeWiring{
            .logger = logger,
            .eventSink = eventSink,
            .inputSource = inputSource,
            .width = 4,
            .height = 4,
            .maxTicks = 10}).board;

    EXPECT_EQ(board.alive, std::vector<bool>(16, false));
}

TEST(BootstrapTest, Bootstrap_ForwardsDispatchedEventsToATickEventRecorder)
{
    NiceMock<MockLogger> logger;
    NiceMock<MockEventSink> eventSink;

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
        antwika::life::LifeWiring{
            .logger = logger,
            .eventSink = eventSink,
            .inputSource = inputSource,
            .width = 4,
            .height = 4,
            .maxTicks = 10,
            .replayRecorder = replayRecorder});

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

TEST(BootstrapTest, Bootstrap_ThrowsWhenMaxTicksIsReachedWithoutAStopEvent)
{
    NiceMock<MockLogger> logger;
    NiceMock<MockEventSink> eventSink;

    ReplaySource inputSource({});

    EXPECT_THROW(
        antwika::life::bootstrap(
            antwika::life::LifeWiring{
                .logger = logger,
                .eventSink = eventSink,
                .inputSource = inputSource,
                .width = 4,
                .height = 4,
                .maxTicks = 3}),
        EngineLoopError);
}

namespace
{
}

TEST(ObserversForTest, ObserversFor_LeaveTheBoardUnprintedWhenDrawing)
{
    NiceMock<MockSystem> renderer;
    NiceMock<MockSystem> printer;
    NiceMock<MockSystem> pacer;

    const auto observers = antwika::life::observersFor(
        renderer, printer, pacer, false);

    ASSERT_EQ(observers.size(), 2U);
    EXPECT_EQ(&observers[0].get(), &renderer);
    EXPECT_EQ(&observers[1].get(), &pacer);
}

TEST(ObserversForTest, ObserversFor_PrintTheBoardWhenNotDrawing)
{
    NiceMock<MockSystem> renderer;
    NiceMock<MockSystem> printer;
    NiceMock<MockSystem> pacer;

    const auto observers = antwika::life::observersFor(
        renderer, printer, pacer, true);

    ASSERT_EQ(observers.size(), 3U);
    EXPECT_EQ(&observers[0].get(), &renderer);
    EXPECT_EQ(&observers[1].get(), &printer);

    EXPECT_EQ(&observers[2].get(), &pacer);
}

TEST(AnnounceHowToStopTest, AnnounceHowToStop_SaysNothingWithAWindow)
{
    NiceMock<MockLogger> logger;

    EXPECT_CALL(logger, log(::testing::_, ::testing::_)).Times(0);

    antwika::life::announceHowToStop(logger, false);
}

TEST(AnnounceHowToStopTest, AnnounceHowToStop_SaysHowWhenHeadless)
{
    NiceMock<MockLogger> logger;

    EXPECT_CALL(
        logger,
        log(::testing::_, ::testing::HasSubstr("press Ctrl+C to stop")));

    antwika::life::announceHowToStop(logger, true);
}

TEST(LifeSummaryTest, OperatorEquals_ComparesEveryField)
{
    antwika::life::LifeSummary base;
    base.board = Board{
        .width = 2, .height = 1, .alive = {true, false}};
    base.console = {"> dump_state"};

    const auto twin = base;
    EXPECT_EQ(base, twin);

    auto reboarded = base;
    reboarded.board.alive[0] = false;
    EXPECT_NE(base, reboarded);

    auto silenced = base;
    silenced.console.clear();
    EXPECT_NE(base, silenced);
}
