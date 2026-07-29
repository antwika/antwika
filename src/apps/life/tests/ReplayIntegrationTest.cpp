#include <gtest/gtest.h>

#include <sstream>

#include <antwika/engine/Events.hpp>
#include <antwika/event/Event.hpp>
#include <antwika/event/EventRecorder.hpp>
#include <antwika/event/TickEvent.hpp>
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
using antwika::event::EventRecorder;
using antwika::event::TickEvent;
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
    constexpr antwika::time::Tick kMaxTicks = 10;
    constexpr std::uint32_t kWidth = 5;
    constexpr std::uint32_t kHeight = 5;

    Board runLife(IReplaySource &source)
    {
        std::chrono::system_clock::time_point time{};
        FakeClock fakeClock(time);
        NullAppender appender;
        PlainFormatter formatter;
        MinimumLevelLogPolicy logPolicy(Level::Info);
        EventRecorder eventSink;

        return antwika::life::bootstrap(
            fakeClock,
            appender,
            formatter,
            logPolicy,
            eventSink,
            source,
            kWidth,
            kHeight,
            {},
            kMaxTicks);
    }
} // namespace

// This is the requirement this project exists for.
// Save a replay from a live run, then load it back.
// Prove the simulation reaches exactly the same board.
// Both runs go through the real antwika::life::bootstrap() entry point.
// That's the same entry point main.cpp uses, not a test-only shortcut.
TEST(ReplayIntegrationTest, LoadingASavedReplayReproducesTheSameBoard)
{
    std::vector<TickEvent> script{
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
