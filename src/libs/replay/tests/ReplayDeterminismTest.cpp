#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstdint>
#include <sstream>
#include <string>

#include <antwika/engine/Engine.hpp>
#include <antwika/event/EventDispatcher.hpp>
#include <antwika/event/EventQueue.hpp>
#include <antwika/event/ITimedEventSink.hpp>
#include <antwika/event/ReplayRecorder.hpp>
#include <antwika/event/TickedEventDispatcher.hpp>
#include <antwika/log/mocks/MockLogger.hpp>

#include "antwika/replay/BinaryEventCodec.hpp"
#include "antwika/replay/BinaryReplayReader.hpp"
#include "antwika/replay/BinaryReplayWriter.hpp"
#include "antwika/replay/EngineLoop.hpp"
#include "antwika/replay/ReplaySource.hpp"

using antwika::engine::Engine;
using antwika::event::Event;
using antwika::event::EventDispatcher;
using antwika::event::EventQueue;
using antwika::event::ITimedEventSink;
using antwika::event::ReplayRecorder;
using antwika::event::TickedEventDispatcher;
using antwika::event::TimedEvent;
using antwika::log::mocks::MockLogger;
using antwika::replay::BinaryEventCodec;
using antwika::replay::BinaryReplayReader;
using antwika::replay::BinaryReplayWriter;
using antwika::replay::EngineLoop;
using antwika::replay::ReplaySource;

namespace
{
    // A minimal, test-only "folded state" -- deliberately not production code.
    // Gives the determinism assertion something concrete to compare.
    // It goes beyond just "the same events came out".
    // It's independent of the real GameState demo in apps/game.
    class FoldingStateReducer final : public ITimedEventSink
    {
    public:
        void handle(const TimedEvent &event) override
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

    std::uint64_t runScriptedTicks(std::vector<TimedEvent> scriptedEvents,
                                   antwika::time::Tick totalTicks,
                                   ReplayRecorder &recorder)
    {
        MockLogger mockLogger;
        EXPECT_CALL(mockLogger, log(::testing::_, ::testing::_)).Times(::testing::AnyNumber());

        EventQueue eventQueue;
        EventDispatcher plainDispatcher(eventQueue, {});
        FoldingStateReducer reducer;
        TickedEventDispatcher tickedDispatcher(plainDispatcher, {recorder, reducer});
        Engine engine(mockLogger, eventQueue, tickedDispatcher);
        ReplaySource source(std::move(scriptedEvents));
        EngineLoop loop(engine, tickedDispatcher, source);

        loop.run(totalTicks);

        return reducer.hash();
    }
} // namespace

// A replay's *input* is what must be fed back in to reproduce a run.
// This differs from a run's *full observed history*.
// That's the complete audit trail, including self-generated events.
// One example is the engine's own built-in tick event.
// Engine::step() dispatches its own engine.tick event fresh every tick.
// It does this whether the run is live or replayed.
// That's what makes it usable via the ordinary ITimedEventSink mechanism.
// The replay system doesn't need to know it's "special".
// But that also means it must never be fed back in as replay input itself.
// Doing so would dispatch it twice per tick during replay.
// It would run once from the source and once from Engine::step().
// That breaks determinism instead of proving it.
// What gets serialized here is the original input script.
// That's scriptedLiveEvents, not liveRecording's full history.
// liveRecording's history is a strictly larger, derived set.
// See blog/2026-07-27-building-a-deterministic-replay-system.md for details.
TEST(ReplayDeterminismTest, LoadingAReplayReproducesTheSameStateAsTheOriginalRun)
{
    constexpr antwika::time::Tick totalTicks = 3;
    std::vector<TimedEvent> scriptedLiveEvents{
        TimedEvent{.tick = 1, .event = Event{.name = "game.score_increment", .payload = "amount=5"}},
    };

    ReplayRecorder liveRecording;
    const auto liveStateHash = runScriptedTicks(scriptedLiveEvents, totalTicks, liveRecording);

    BinaryEventCodec codec;
    BinaryReplayWriter writer(codec);
    std::stringstream replayStream;
    writer.write(scriptedLiveEvents, replayStream);

    BinaryReplayReader reader(codec);
    auto loadedInputEvents = reader.read(replayStream);
    EXPECT_EQ(loadedInputEvents, scriptedLiveEvents);

    ReplayRecorder replayedRecording;
    const auto replayedStateHash = runScriptedTicks(loadedInputEvents, totalTicks, replayedRecording);

    EXPECT_EQ(replayedStateHash, liveStateHash);
    EXPECT_EQ(replayedRecording.getEvents(), liveRecording.getEvents());
}

TEST(ReplayDeterminismTest, SerializingTheSameRecordingTwiceProducesIdenticalBytes)
{
    constexpr antwika::time::Tick totalTicks = 3;
    std::vector<TimedEvent> scriptedLiveEvents{
        TimedEvent{.tick = 1, .event = Event{.name = "game.score_increment", .payload = "amount=5"}},
        TimedEvent{.tick = 2, .event = Event{.name = "game.score_increment", .payload = "amount=2"}},
    };

    ReplayRecorder recording;
    (void)runScriptedTicks(scriptedLiveEvents, totalTicks, recording);

    BinaryEventCodec codec;
    BinaryReplayWriter writer(codec);

    std::stringstream firstSerialization;
    writer.write(recording.getEvents(), firstSerialization);

    std::stringstream secondSerialization;
    writer.write(recording.getEvents(), secondSerialization);

    EXPECT_EQ(firstSerialization.str(), secondSerialization.str());
}
