#include <gtest/gtest.h>

#include <sstream>

#include <antwika/event/Event.hpp>
#include <antwika/event/EventQueue.hpp>
#include <antwika/event/EventRecorder.hpp>
#include <antwika/event/TimedEvent.hpp>
#include <antwika/log/Level.hpp>
#include <antwika/log/MinimumLevelLogPolicy.hpp>
#include <antwika/log/NullAppender.hpp>
#include <antwika/log/PlainFormatter.hpp>
#include <antwika/replay/BinaryEventCodec.hpp>
#include <antwika/replay/BinaryReplayReader.hpp>
#include <antwika/replay/BinaryReplayWriter.hpp>
#include <antwika/replay/IReplaySource.hpp>
#include <antwika/replay/ReplaySource.hpp>
#include <antwika/time/fakes/FakeClock.hpp>

#include "antwika/life/Events.hpp"
#include "antwika/life/Life.hpp"

using antwika::event::Event;
using antwika::event::EventQueue;
using antwika::event::EventRecorder;
using antwika::event::TimedEvent;
using antwika::life::Board;
using antwika::log::Level;
using antwika::log::MinimumLevelLogPolicy;
using antwika::log::NullAppender;
using antwika::log::PlainFormatter;
using antwika::replay::BinaryEventCodec;
using antwika::replay::BinaryReplayReader;
using antwika::replay::BinaryReplayWriter;
using antwika::replay::IReplaySource;
using antwika::replay::ReplaySource;
using antwika::time::fakes::FakeClock;

namespace
{
    constexpr antwika::time::Tick kTotalTicks = 4;
    constexpr std::uint32_t kWidth = 5;
    constexpr std::uint32_t kHeight = 5;

    Board runLife(IReplaySource &source)
    {
        std::chrono::system_clock::time_point time{};
        FakeClock fakeClock(time);
        NullAppender appender;
        PlainFormatter formatter;
        MinimumLevelLogPolicy logPolicy(Level::Info);
        EventQueue eventQueue;
        EventRecorder eventSink;

        return antwika::life::bootstrap(
            fakeClock,
            appender,
            formatter,
            logPolicy,
            eventQueue,
            eventSink,
            source,
            kTotalTicks,
            kWidth,
            kHeight);
    }
} // namespace

// This is the requirement this project exists for.
// Save a replay from a live run, then load it back.
// Prove the simulation reaches exactly the same board.
// Both runs go through the real antwika::life::bootstrap() entry point.
// That's the same entry point main.cpp uses, not a test-only shortcut.
TEST(ReplayIntegrationTest, LoadingASavedReplayReproducesTheSameBoard)
{
    std::vector<TimedEvent> script{
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
    };

    ReplaySource liveSource(script);
    auto liveBoard = runLife(liveSource);

    BinaryEventCodec codec;
    BinaryReplayWriter writer(codec);
    std::stringstream replayStream;
    writer.write(script, replayStream);

    BinaryReplayReader reader(codec);
    auto loadedEvents = reader.read(replayStream);
    ReplaySource replaySource(loadedEvents);
    auto replayedBoard = runLife(replaySource);

    EXPECT_EQ(replayedBoard, liveBoard);
}
