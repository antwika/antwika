#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <sstream>

#include <antwika/engine/Events.hpp>
#include <antwika/event/Event.hpp>
#include <antwika/event/EventRecorder.hpp>
#include <antwika/event/TickEvent.hpp>
#include <antwika/log/mocks/MockLogger.hpp>
#include <antwika/replay/IReplaySource.hpp>
#include <antwika/replay/ReplayReader.hpp>
#include <antwika/replay/ReplaySource.hpp>
#include <antwika/replay/ReplayWriter.hpp>

#include "antwika/life/Events.hpp"
#include "antwika/life/Life.hpp"

using antwika::event::Event;
using antwika::event::EventRecorder;
using antwika::event::TickEvent;
using antwika::life::Board;
using antwika::log::mocks::MockLogger;
using antwika::replay::IReplaySource;
using antwika::replay::ReplayReader;
using antwika::replay::ReplaySource;
using antwika::replay::ReplayWriter;
using ::testing::NiceMock;

namespace
{
    constexpr antwika::time::Tick kMaxTicks = 10;
    constexpr std::uint32_t kWidth = 5;
    constexpr std::uint32_t kHeight = 5;

    Board runLife(IReplaySource &source)
    {
        NiceMock<MockLogger> logger;
        EventRecorder eventSink;

        return antwika::life::bootstrap(
            antwika::life::LifeConfig{
                .logger = logger,
                .eventSink = eventSink,
                .inputSource = source,
                .width = kWidth,
                .height = kHeight,
                .maxTicks = kMaxTicks});
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

    ReplayWriter writer;
    std::stringstream replayStream;
    writer.write(script, replayStream);

    ReplayReader reader;
    auto loadedEvents = reader.read(replayStream);
    ReplaySource replaySource(loadedEvents);
    auto replayedBoard = runLife(replaySource);

    EXPECT_EQ(replayedBoard, liveBoard);
}
