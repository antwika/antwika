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
#include <antwika/replay/IReplaySource.hpp>
#include <antwika/replay/ReplayReader.hpp>
#include <antwika/replay/ReplaySource.hpp>
#include <antwika/replay/ReplayWriter.hpp>
#include <antwika/time/fakes/FakeClock.hpp>

#include "antwika/game/Events.hpp"
#include "antwika/game/Game.hpp"

using antwika::event::Event;
using antwika::event::EventRecorder;
using antwika::event::TickEvent;
using antwika::game::GameState;
using antwika::log::Level;
using antwika::log::MinimumLevelLogPolicy;
using antwika::log::NullAppender;
using antwika::log::PlainFormatter;
using antwika::replay::IReplaySource;
using antwika::replay::ReplayReader;
using antwika::replay::ReplaySource;
using antwika::replay::ReplayWriter;
using antwika::time::fakes::FakeClock;

namespace
{
    constexpr antwika::time::Tick kMaxTicks = 10;

    GameState runGame(IReplaySource &source)
    {
        std::chrono::system_clock::time_point time{};
        FakeClock fakeClock(time);
        NullAppender appender;
        PlainFormatter formatter;
        MinimumLevelLogPolicy logPolicy(Level::Info);
        EventRecorder eventSink;

        return antwika::game::bootstrap(
            fakeClock,
            appender,
            formatter,
            logPolicy,
            eventSink,
            source,
            kMaxTicks);
    }
} // namespace

// This is the requirement this project exists for.
// Save a replay from a live run, then load it back.
// Prove the game reaches exactly the same state.
// Both runs go through the real antwika::game::bootstrap() entry point.
// That's the same entry point main.cpp uses, not a test-only shortcut.
TEST(ReplayIntegrationTest, LoadingASavedReplayReproducesTheSameGameState)
{
    std::vector<TickEvent> script{
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
    };

    ReplaySource liveSource(script);
    auto liveState = runGame(liveSource);

    ReplayWriter writer;
    std::stringstream replayStream;
    writer.write(script, replayStream);

    ReplayReader reader;
    auto loadedEvents = reader.read(replayStream);
    ReplaySource replaySource(loadedEvents);
    auto replayedState = runGame(replaySource);

    EXPECT_EQ(replayedState, liveState);
    EXPECT_EQ(replayedState, (GameState{.ticksProcessed = 5, .score = 7}));
}
