#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <sstream>

#include <antwika/engine/Events.hpp>
#include <antwika/event/Event.hpp>
#include <antwika/event/mocks/MockEventSink.hpp>
#include <antwika/event/TickEvent.hpp>
#include <antwika/input/InputEventCodec.hpp>
#include <antwika/log/mocks/MockLogger.hpp>
#include <antwika/simulation/ITickEventSource.hpp>
#include <antwika/replay/ReplayReader.hpp>
#include <antwika/replay/ReplaySource.hpp>
#include <antwika/replay/ReplayWriter.hpp>

#include "antwika/game/AppMode.hpp"
#include "antwika/game/BuildingIndex.hpp"
#include "antwika/game/Camera.hpp"
#include "antwika/game/Events.hpp"
#include "antwika/game/Game.hpp"
#include "antwika/game/GridExtent.hpp"
#include "antwika/game/PathIndex.hpp"
#include "antwika/game/PauseState.hpp"

using antwika::event::Event;
using antwika::event::mocks::MockEventSink;
using antwika::event::TickEvent;
using antwika::game::AppMode;
using antwika::game::AppModeState;
using antwika::game::Camera;
using antwika::game::GameState;
using antwika::game::GridExtent;
using antwika::game::PathIndex;
using antwika::input::InputEventCodec;
using antwika::log::mocks::MockLogger;
using antwika::simulation::ITickEventSource;
using antwika::replay::ReplayReader;
using antwika::replay::ReplaySource;
using antwika::replay::ReplayWriter;
using ::testing::NiceMock;

namespace
{
    constexpr antwika::time::Tick kMaxTicks = 10;

    GameState runGame(ITickEventSource &source)
    {
        NiceMock<MockLogger> logger;
        NiceMock<MockEventSink> eventSink;
        const InputEventCodec codec;
        Camera camera;
        PathIndex paths;
        antwika::game::BuildingIndex built;

        // Both runs start in the same mode, which is the point:
        // nothing about being a replay may change what a click means.
        AppModeState mode{AppMode::CityMap};
        antwika::game::PauseState pause;

        return antwika::game::bootstrap(
                   antwika::game::GameConfig{
                       .logger = logger,
                       .eventSink = eventSink,
                       .inputSource = source,
                       .codec = codec,
                       .extent = GridExtent{.width = 16, .height = 16},
                       .camera = camera,
                       .paths = paths,
                       .built = built,
                       .mode = mode,
                       .pause = pause,
                       .maxTicks = kMaxTicks})
            .state;
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
