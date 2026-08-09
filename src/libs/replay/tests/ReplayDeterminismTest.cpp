#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstdint>
#include <sstream>
#include <string>
#include <vector>

#include <antwika/engine/Engine.hpp>
#include <antwika/engine/Events.hpp>
#include <antwika/engine/StopSignal.hpp>
#include <antwika/event/EventDispatcher.hpp>
#include <antwika/event/ITickEventSink.hpp>
#include <antwika/event/TickEventRecorder.hpp>
#include <antwika/event/TickedEventDispatcher.hpp>
#include <antwika/log/mocks/MockLogger.hpp>
#include <antwika/replay/fakes/FakeFoldingStateReducer.hpp>

#include "antwika/simulation/EngineLoop.hpp"
#include "antwika/replay/ReplayReader.hpp"
#include "antwika/replay/ReplaySource.hpp"
#include "antwika/replay/ReplayWriter.hpp"

using antwika::engine::Engine;
using antwika::engine::StopSignal;
using antwika::event::Event;
using antwika::event::EventDispatcher;
using antwika::event::ITickEventSink;
using antwika::event::TickEventRecorder;
using antwika::event::TickedEventDispatcher;
using antwika::event::TickEvent;
using antwika::log::mocks::MockLogger;
using antwika::replay::fakes::FakeFoldingStateReducer;
using antwika::simulation::EngineLoop;
using antwika::replay::ReplayReader;
using antwika::replay::ReplaySource;
using antwika::replay::ReplayWriter;

namespace
{
    constexpr std::uint64_t kScriptedRunHash = 12740693313303563000ULL;

    std::uint64_t runScriptedTicks(
        std::vector<TickEvent> scriptedEvents,
        TickEventRecorder &recorder,
        antwika::time::Tick maxTicks)
    {
        MockLogger mockLogger;
        EXPECT_CALL(mockLogger, log(::testing::_, ::testing::_))
            .Times(::testing::AnyNumber());

        EventDispatcher plainDispatcher({});
        FakeFoldingStateReducer reducer;
        StopSignal stopSignal;
        TickedEventDispatcher tickedDispatcher(
            plainDispatcher, {recorder, reducer, stopSignal});
        Engine engine(mockLogger, tickedDispatcher);
        ReplaySource source(std::move(scriptedEvents));
        EngineLoop loop(engine, tickedDispatcher, source);

        loop.run(stopSignal, maxTicks);

        return reducer.hash();
    }
}

TEST(
    ReplayDeterminismTest,
    Read_ReproducesTheOriginalRunsState)
{
    constexpr antwika::time::Tick maxTicks = 10;
    std::vector<TickEvent> scriptedLiveEvents{
        TickEvent{
            .tick = 1,
            .event = Event{
                .name = "game.score_increment",
                .payload = "amount=5",
            },
        },
        TickEvent{
            .tick = 2,
            .event = Event{.name = antwika::engine::events::kStop},
        },
    };

    TickEventRecorder liveRecording;
    const auto liveStateHash =
        runScriptedTicks(scriptedLiveEvents, liveRecording, maxTicks);

    ReplayWriter writer;
    std::stringstream replayStream;
    writer.write(scriptedLiveEvents, replayStream);

    ReplayReader reader;
    auto loadedInputEvents = reader.read(replayStream);
    ASSERT_EQ(loadedInputEvents, scriptedLiveEvents);

    TickEventRecorder replayedRecording;
    const auto replayedStateHash =
        runScriptedTicks(loadedInputEvents, replayedRecording, maxTicks);

    EXPECT_EQ(liveStateHash, kScriptedRunHash);
    EXPECT_EQ(replayedStateHash, liveStateHash);
    EXPECT_EQ(replayedRecording.getEvents(), liveRecording.getEvents());
}

TEST(
    ReplayDeterminismTest,
    Read_MatchesJsonLinesForAWholeDocument)
{
    constexpr antwika::time::Tick maxTicks = 10;
    const std::vector<TickEvent> scriptedLiveEvents{
        TickEvent{
            .tick = 1,
            .event = Event{
                .name = "game.score_increment",
                .payload = "amount=5",
            },
        },
        TickEvent{
            .tick = 2,
            .event = Event{.name = antwika::engine::events::kStop},
        },
    };

    TickEventRecorder liveRecording;
    const auto liveStateHash =
        runScriptedTicks(scriptedLiveEvents, liveRecording, maxTicks);

    std::stringstream wholeDocument(
        R"({"magic":"antwika-replay","version":1,"events":[)"
        R"({"tick":1,"event":{"name":"game.score_increment",)"
        R"("payload":"amount=5"}},)"
        R"({"tick":2,"event":{"name":"engine.stop","payload":""}}]})");

    const ReplayReader reader;
    const auto loaded = reader.read(wholeDocument);
    ASSERT_EQ(loaded, scriptedLiveEvents);

    TickEventRecorder replayedRecording;
    const auto replayedStateHash =
        runScriptedTicks(loaded, replayedRecording, maxTicks);

    EXPECT_EQ(liveStateHash, kScriptedRunHash);
    EXPECT_EQ(replayedStateHash, liveStateHash);
    EXPECT_EQ(replayedRecording.getEvents(), liveRecording.getEvents());
}

TEST(
    ReplayDeterminismTest,
    Write_StaysValidWithBuiltInTicksRemoved)
{
    constexpr antwika::time::Tick maxTicks = 10;
    std::vector<TickEvent> scriptedLiveEvents{
        TickEvent{
            .tick = 1,
            .event = Event{
                .name = "game.score_increment",
                .payload = "amount=5",
            },
        },
        TickEvent{
            .tick = 2,
            .event = Event{.name = antwika::engine::events::kStop},
        },
    };

    TickEventRecorder liveRecording;
    const auto liveStateHash =
        runScriptedTicks(scriptedLiveEvents, liveRecording, maxTicks);

    auto derivedInput = liveRecording.getEvents();
    std::erase_if(
        derivedInput,
        [](const TickEvent &event)
        {
            return event.event.name == antwika::engine::events::kTick;
        });
    EXPECT_EQ(derivedInput, scriptedLiveEvents);

    TickEventRecorder replayedRecording;
    const auto replayedStateHash =
        runScriptedTicks(derivedInput, replayedRecording, maxTicks);

    EXPECT_EQ(liveStateHash, kScriptedRunHash);
    EXPECT_EQ(replayedStateHash, liveStateHash);
}
