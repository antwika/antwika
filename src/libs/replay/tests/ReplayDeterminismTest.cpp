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
using antwika::simulation::EngineLoop;
using antwika::replay::ReplayReader;
using antwika::replay::ReplaySource;
using antwika::replay::ReplayWriter;

namespace
{
    // A minimal, test-only "folded state" -- deliberately not production code.
    // Gives the determinism assertion something concrete to compare.
    // It goes beyond just "the same events came out".
    // It's independent of the real GameState demo in apps/game.
    class FoldingStateReducer final : public ITickEventSink
    {
    public:
        void handle(const TickEvent &event) override
        {
            fold(event.tick);
            fold(event.event.name);
            fold(event.event.payload);
        }

        [[nodiscard]] std::uint64_t hash() const noexcept
        {
            return state;
        }

    private:
        void fold(std::uint64_t value)
        {
            for (int i = 0; i < 8; ++i)
            {
                fold(static_cast<unsigned char>((value >> (i * 8)) & 0xFF));
            }
        }

        void fold(const std::string &value)
        {
            for (unsigned char byte : value)
            {
                fold(byte);
            }
        }

        void fold(unsigned char byte)
        {
            state ^= byte;
            state *= 1099511628211ULL; // FNV-1a prime
        }

        std::uint64_t state{14695981039346656037ULL}; // FNV-1a offset basis
    };

    std::uint64_t runScriptedTicks(
        std::vector<TickEvent> scriptedEvents,
        TickEventRecorder &recorder,
        antwika::time::Tick maxTicks)
    {
        MockLogger mockLogger;
        EXPECT_CALL(mockLogger, log(::testing::_, ::testing::_))
            .Times(::testing::AnyNumber());

        EventDispatcher plainDispatcher({});
        FoldingStateReducer reducer;
        StopSignal stopSignal;
        TickedEventDispatcher tickedDispatcher(
            plainDispatcher, {recorder, reducer, stopSignal});
        Engine engine(mockLogger, tickedDispatcher);
        ReplaySource source(std::move(scriptedEvents));
        EngineLoop loop(engine, tickedDispatcher, source);

        loop.run(stopSignal, maxTicks);

        return reducer.hash();
    }
} // namespace

// A replay's *input* is what must be fed back in to reproduce a run.
// This differs from a run's *full observed history*.
// That's the complete audit trail, including self-generated events.
// One example is the engine's own built-in tick event.
// Engine::step() dispatches its own engine.tick event fresh every tick.
// It does this whether the run is live or replayed.
// That's what makes it usable via the ordinary ITickEventSink mechanism.
// The replay system doesn't need to know it's "special".
// But that also means it must never be fed back in as replay input itself.
// Doing so would dispatch it twice per tick during replay.
// It would run once from the source and once from Engine::step().
// That breaks determinism instead of proving it.
// What gets serialized here is the original input script.
// That's scriptedLiveEvents, not liveRecording's full history.
// liveRecording's history is a strictly larger, derived set.
// See blog/001-building-a-deterministic-replay-system.md for details.
TEST(
    ReplayDeterminismTest,
    LoadingAReplayReproducesTheSameStateAsTheOriginalRun)
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
    EXPECT_EQ(loadedInputEvents, scriptedLiveEvents);

    TickEventRecorder replayedRecording;
    const auto replayedStateHash =
        runScriptedTicks(loadedInputEvents, replayedRecording, maxTicks);

    EXPECT_EQ(replayedStateHash, liveStateHash);
    EXPECT_EQ(replayedRecording.getEvents(), liveRecording.getEvents());
}

TEST(
    ReplayDeterminismTest,
    SerializingTheSameRecordingTwiceProducesIdenticalBytes)
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
            .event = Event{
                .name = "game.score_increment",
                .payload = "amount=2",
            },
        },
        TickEvent{
            .tick = 3,
            .event = Event{.name = antwika::engine::events::kStop},
        },
    };

    TickEventRecorder recording;
    (void)runScriptedTicks(scriptedLiveEvents, recording, maxTicks);

    ReplayWriter writer;

    std::stringstream firstSerialization;
    writer.write(recording.getEvents(), firstSerialization);

    std::stringstream secondSerialization;
    writer.write(recording.getEvents(), secondSerialization);

    EXPECT_EQ(firstSerialization.str(), secondSerialization.str());
}

// A genuinely live run has no pre-known input script.
// Unlike this suite's stand-ins, it has nothing to hand `--record`.
// The recorder's full history is all there is to build replay input from.
// Filtering engine.tick out of that history must still reproduce the run.
// This is the same guarantee as the test above, proven in reverse.
// It derives replay input from a recording, not from a known script.
TEST(
    ReplayDeterminismTest,
    FilteringBuiltInTicksFromARecordingYieldsValidReplayInput)
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

    EXPECT_EQ(replayedStateHash, liveStateHash);
}
