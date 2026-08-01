#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <filesystem>
#include <string>
#include <system_error>
#include <vector>

#include <antwika/engine/Events.hpp>
#include <antwika/event/Event.hpp>
#include <antwika/event/mocks/MockEventSink.hpp>
#include <antwika/event/TickEvent.hpp>
#include <antwika/event/TickEventRecorder.hpp>
#include <antwika/gfx/NullBackend.hpp>
#include <antwika/gfx/WindowDesc.hpp>
#include <antwika/gfx/WindowEvent.hpp>
#include <antwika/gfx/WindowId.hpp>
#include <antwika/gfx/mocks/MockGfxBackend.hpp>
#include <antwika/input/IdleMotionSource.hpp>
#include <antwika/input/InputEvent.hpp>
#include <antwika/input/InputEventCodec.hpp>
#include <antwika/input/MouseButton.hpp>
#include <antwika/input/Position.hpp>
#include <antwika/ui/Pointer.hpp>
#include <antwika/ui/WidgetId.hpp>
#include <antwika/log/mocks/MockLogger.hpp>
#include <antwika/replay/ReplayCli.hpp>
#include <antwika/replay/ReplaySource.hpp>

#include "antwika/game/AppMode.hpp"
#include "antwika/game/BuildingIndex.hpp"
#include "antwika/game/Camera.hpp"
#include "antwika/game/BuildTool.hpp"
#include "antwika/game/Cell.hpp"
#include "antwika/game/Events.hpp"
#include "antwika/game/Game.hpp"
#include "antwika/game/GridExtent.hpp"
#include "antwika/game/IsoProjection.hpp"
#include "antwika/game/PathIndex.hpp"
#include "antwika/game/PauseState.hpp"
#include "antwika/game/Toolbar.hpp"
#include "antwika/game/UiCanvas.hpp"
#include "antwika/game/UiOverlay.hpp"
#include "antwika/game/WindowInputSource.hpp"

using antwika::event::Event;
using antwika::event::mocks::MockEventSink;
using antwika::event::TickEvent;
using antwika::event::TickEventRecorder;
using antwika::game::AppMode;
using antwika::game::AppModeState;
using antwika::game::Camera;
using antwika::game::Cell;
using antwika::game::cellCentre;
using antwika::game::GameSummary;
using antwika::game::GridExtent;
using antwika::game::kUiCanvas;
using antwika::game::PathIndex;
using antwika::game::Toolbar;
using antwika::game::UiOverlay;
using antwika::game::WindowInputSource;
using antwika::gfx::CloseRequested;
using antwika::gfx::NullBackend;
using antwika::gfx::WindowEvent;
using antwika::gfx::WindowId;
using antwika::gfx::mocks::MockGfxBackend;
using antwika::input::IdleMotionSource;
using antwika::input::InputEventCodec;
using antwika::input::MouseButton;
using antwika::input::PointerButtonPressed;
using antwika::input::PointerButtonReleased;
using antwika::input::PointerMoved;
using antwika::input::PointerScrolled;
using antwika::log::mocks::MockLogger;
using antwika::replay::ReplaySource;
using ::testing::NiceMock;
using ::testing::Return;

namespace
{
    constexpr GridExtent kExtent{.width = 16, .height = 16};
    constexpr antwika::time::Tick kMaxTicks = 40;
    constexpr WindowId kWindow{7};

    // Removes its backing file on scope exit.
    class ScratchFile
    {
    public:
        explicit ScratchFile(std::string name)
            : path((std::filesystem::temp_directory_path() / name).string())
        {
        }

        ScratchFile(const ScratchFile &) = delete;
        ScratchFile(ScratchFile &&) = delete;

        ScratchFile &operator=(const ScratchFile &) = delete;
        ScratchFile &operator=(ScratchFile &&) = delete;

        ~ScratchFile()
        {
            // A destructor must not throw, so use the non-throwing form.
            std::error_code ignored;
            std::filesystem::remove(path, ignored);
        }

        [[nodiscard]] const std::string &name() const noexcept
        {
            return path;
        }

    private:
        std::string path;
    };

    struct RunResult
    {
        GameSummary summary;
        std::vector<TickEvent> recorded;
    };

    [[nodiscard]] RunResult run(antwika::simulation::ITickEventSource &source)
    {
        NiceMock<MockLogger> logger;
        NiceMock<MockEventSink> eventSink;
        const InputEventCodec codec;
        Camera camera;
        PathIndex paths;
        antwika::game::BuildingIndex built;
        TickEventRecorder recorder;

        // The subject here is the grid, so a run starts there.
        AppModeState mode{AppMode::CityMap};
        antwika::game::PauseState pause;

        auto summary = antwika::game::bootstrap(
            antwika::game::GameConfig{
                .logger = logger,
                .eventSink = eventSink,
                .inputSource = source,
                .codec = codec,
                .extent = kExtent,
                .camera = camera,
                .paths = paths,
                .built = built,
                .mode = mode,
                .pause = pause,
                .maxTicks = kMaxTicks,
                .replayRecorder = recorder});

        return RunResult{
            .summary = std::move(summary),
            .recorded = recorder.getEvents()};
    }

    [[nodiscard]] RunResult runWithToolbar(
        antwika::simulation::ITickEventSource &source)
    {
        NiceMock<MockLogger> logger;
        NiceMock<MockEventSink> eventSink;
        const InputEventCodec codec;
        Camera camera;
        PathIndex paths;
        antwika::game::BuildingIndex built;
        TickEventRecorder recorder;

        // The subject here is the grid, so a run starts there.
        AppModeState mode{AppMode::CityMap};
        antwika::game::PauseState pause;
        UiOverlay overlay(kUiCanvas);

        auto summary = antwika::game::bootstrap(
            antwika::game::GameConfig{
                .logger = logger,
                .eventSink = eventSink,
                .inputSource = source,
                .codec = codec,
                .extent = kExtent,
                .camera = camera,
                .paths = paths,
                .built = built,
                .mode = mode,
                .pause = pause,
                .maxTicks = kMaxTicks,
                .replayRecorder = recorder,
                .overlay = overlay});

        return RunResult{
            .summary = std::move(summary),
            .recorded = recorder.getEvents()};
    }

    // Where a button is, is the layout's business.
    // So a test looks for a pixel that hits the one it means.
    [[nodiscard]] antwika::input::Position pixelOn(
        antwika::ui::WidgetId id)
    {
        const Toolbar toolbar;
        const Camera camera;

        for (std::int32_t y = 0;
             y < static_cast<std::int32_t>(kUiCanvas.height);
             y += 4)
        {
            for (std::int32_t x = 0;
                 x < static_cast<std::int32_t>(kUiCanvas.width);
                 x += 4)
            {
                const antwika::ui::Pointer pointer{
                    .position = antwika::gfx::Point{.x = x, .y = y}};

                if (toolbar.describe(kUiCanvas, pointer, camera)
                        .interactions.hovered
                    == id)
                {
                    return antwika::input::Position{.x = x, .y = y};
                }
            }
        }

        return antwika::input::Position{};
    }

    [[nodiscard]] std::vector<TickEvent> toolbarSession()
    {
        const InputEventCodec codec;
        const auto at = pixelOn(antwika::game::widgets::kZoomIn);

        return {
            TickEvent{
                .tick = 0,
                .event = codec.encode(
                    antwika::input::PointerMoved{.position = at})},
            TickEvent{
                .tick = 0,
                .event = codec.encode(
                    PointerButtonPressed{
                        .button = MouseButton::Left, .position = at})},
            TickEvent{
                .tick = 2,
                .event = Event{.name = antwika::engine::events::kStop}}};
    }

    [[nodiscard]] Event pressAt(Cell cell, MouseButton button)
    {
        const InputEventCodec codec;
        const auto point = cellCentre(cell, Camera());

        return codec.encode(
            PointerButtonPressed{
                .button = button,
                .position = {.x = point.x, .y = point.y}});
    }

    // The button coming back up where it went down.
    // A road drag holds the run still until it does -- see RoadDrag.
    // So a stream of presses with no release is impossible input.
    // And one whose walkers would never step.
    [[nodiscard]] Event releaseAt(Cell cell, MouseButton button)
    {
        const InputEventCodec codec;
        const auto point = cellCentre(cell, Camera());

        return codec.encode(
            antwika::input::PointerButtonReleased{
                .button = button,
                .position = {.x = point.x, .y = point.y}});
    }

    // A session worth reproducing.
    // A corridor, a junction, a dead end, two walkers, and a zoom.
    [[nodiscard]] std::vector<TickEvent> scriptedSession()
    {
        const InputEventCodec codec;
        std::vector<TickEvent> events;

        for (std::int32_t x = 1; x <= 5; ++x)
        {
            events.push_back(
                TickEvent{
                    .tick = 0,
                    .event = pressAt(
                        Cell{.x = x, .y = 2}, MouseButton::Left)});
        }
        for (std::int32_t y = 3; y <= 4; ++y)
        {
            events.push_back(
                TickEvent{
                    .tick = 0,
                    .event = pressAt(
                        Cell{.x = 3, .y = y}, MouseButton::Left)});
        }

        events.push_back(
            TickEvent{
                .tick = 0,
                .event =
                    releaseAt(Cell{.x = 3, .y = 4}, MouseButton::Left)});

        events.push_back(
            TickEvent{
                .tick = 1,
                .event =
                    pressAt(Cell{.x = 1, .y = 2}, MouseButton::Right)});
        events.push_back(
            TickEvent{
                .tick = 1,
                .event =
                    pressAt(Cell{.x = 5, .y = 2}, MouseButton::Right)});

        // The camera is state, so this has to reproduce too.
        events.push_back(
            TickEvent{
                .tick = 4,
                .event = codec.encode(PointerScrolled{.vertical = -1})});

        events.push_back(
            TickEvent{
                .tick = 8,
                .event = Event{.name = antwika::engine::events::kStop}});

        return events;
    }
} // namespace

// The requirement the whole design rests on.
// The real save and load are what exercise engine.tick filtering.
TEST(ReplayDeterminismTest, ARecordedRunReplaysToTheSameState)
{
    auto script = scriptedSession();
    ReplaySource liveSource(script);
    const auto live = run(liveSource);

    const ScratchFile file("antwika-game-determinism.replay");
    antwika::replay::saveReplayFile(
        live.recorded, file.name());
    auto loaded = antwika::replay::loadReplayFile(file.name());

    ReplaySource replayedSource(std::move(loaded));
    const auto replayed = run(replayedSource);

    EXPECT_EQ(replayed.summary, live.summary);
    EXPECT_EQ(replayed.recorded, live.recorded);
}

// Two runs that both did nothing would agree for the wrong reason.
TEST(ReplayDeterminismTest, TheScriptedRunActuallyBuildsSomething)
{
    auto script = scriptedSession();
    ReplaySource source(script);

    const auto result = run(source);

    EXPECT_EQ(result.summary.paths.size(), 7U);
    ASSERT_EQ(result.summary.walkers.size(), 2U);

    // The zoom happened, and reached the camera rather than a renderer.
    EXPECT_LT(result.summary.camera.zoomLevel(),
              antwika::game::kDefaultZoomLevel);

    // Both walkers moved off where they were dropped.
    EXPECT_NE(result.summary.walkers[0].at, (Cell{.x = 1, .y = 2}));
    EXPECT_NE(result.summary.walkers[1].at, (Cell{.x = 5, .y = 2}));
}

// The rule that keeps a replay honest.
// The click is persisted, and the placement it caused is not.
TEST(ReplayDeterminismTest, TheRecordingHoldsClicksAndNoDerivedPlacement)
{
    auto script = scriptedSession();
    ReplaySource source(script);

    const auto result = run(source);

    const ScratchFile file("antwika-game-recording.replay");
    antwika::replay::saveReplayFile(
        result.recorded, file.name());
    const auto loaded = antwika::replay::loadReplayFile(file.name());

    const InputEventCodec codec;
    std::size_t inputEvents = 0;
    for (const auto &event : loaded)
    {
        EXPECT_NE(event.event.name, antwika::engine::events::kTick);

        // Nothing named game.place_* may ever appear here.
        EXPECT_EQ(event.event.name.rfind("game.place", 0), std::string::npos)
            << event.event.name;

        if (codec.decode(event.event).has_value())
        {
            ++inputEvents;
        }
    }

    // Every click and the scroll, and nothing derived from them.
    EXPECT_EQ(inputEvents, scriptedSession().size() - 1);
}

// Closing the window is input, so it ends the run and is recorded.
// A replay of that file then stops at the same tick.
TEST(ReplayDeterminismTest, ClosingTheWindowEndsTheRunAndReplaysTheSame)
{
    constexpr antwika::time::Tick kClosedOn = 3;

    NiceMock<MockGfxBackend> backend;
    std::size_t polls = 0;
    ON_CALL(backend, pollEvent())
        .WillByDefault(
            [&polls]() -> std::optional<WindowEvent>
            {
                // One close, on the tick it was asked for, then nothing.
                if (polls++ == kClosedOn)
                {
                    return WindowEvent{
                        .window = kWindow, .payload = CloseRequested{}};
                }
                return std::nullopt;
            });

    auto script = scriptedSession();
    // Drop the scripted stop, so the close is what ends this run.
    script.pop_back();

    ReplaySource fileSource(script);
    WindowInputSource closing(fileSource, backend, kWindow);
    const auto live = run(closing);

    ASSERT_FALSE(live.recorded.empty());
    EXPECT_EQ(
        live.recorded.back().event.name, antwika::engine::events::kTick);

    const ScratchFile file("antwika-game-close.replay");
    antwika::replay::saveReplayFile(
        live.recorded, file.name());
    auto loaded = antwika::replay::loadReplayFile(file.name());

    // Replayed under a source with no window at all.
    ReplaySource replayedSource(std::move(loaded));
    const auto replayed = run(replayedSource);

    EXPECT_EQ(replayed.summary, live.summary);
}

// A click on the toolbar is input like any other.
// What is recorded is the click.
// Which button it hit is worked out again on replay.
TEST(ReplayDeterminismTest, AToolbarClickReplaysToTheSameCamera)
{
    auto script = toolbarSession();
    ReplaySource liveSource(script);
    const auto live = runWithToolbar(liveSource);

    // The press reached the camera, and nothing else.
    EXPECT_EQ(
        antwika::game::kDefaultZoomLevel + 1,
        live.summary.camera.zoomLevel());
    EXPECT_TRUE(live.summary.paths.empty());

    const ScratchFile file("antwika-game-toolbar.replay");
    antwika::replay::saveReplayFile(
        live.recorded, file.name());
    auto loaded = antwika::replay::loadReplayFile(file.name());

    // Nothing about a button may be in the file.
    const InputEventCodec codec;
    for (const auto &event : loaded)
    {
        EXPECT_EQ(event.event.name.rfind("ui.", 0), std::string::npos)
            << event.event.name;
        EXPECT_EQ(event.event.name.rfind("game.zoom", 0), std::string::npos)
            << event.event.name;
        EXPECT_TRUE(
            codec.decode(event.event).has_value()
            || event.event.name == antwika::engine::events::kStop)
            << event.event.name;
    }

    ReplaySource replayedSource(std::move(loaded));
    const auto replayed = runWithToolbar(replayedSource);

    EXPECT_EQ(replayed.summary, live.summary);
    EXPECT_EQ(replayed.recorded, live.recorded);
}

// The toolbar is over the grid, so a press on it is not a press on it.
TEST(ReplayDeterminismTest, AToolbarClickLaysNoPathUnderTheBar)
{
    auto script = toolbarSession();
    ReplaySource source(script);

    const auto result = runWithToolbar(source);

    EXPECT_TRUE(result.summary.paths.empty());
}

namespace
{
    [[nodiscard]] Event moveTo(std::int32_t x, std::int32_t y)
    {
        const InputEventCodec codec;
        return codec.encode(PointerMoved{.position = {.x = x, .y = y}});
    }

    // The session the idle-motion gate exists for.
    // A wander across the window, then a middle-drag that pans.
    // Then another wander, and a zoom anchored where it left off.
    [[nodiscard]] std::vector<TickEvent> wanderingSession()
    {
        const InputEventCodec codec;
        std::vector<TickEvent> events;
        antwika::time::Tick tick = 0;

        const auto wander =
            [&events, &tick](std::int32_t from, std::int32_t to)
        {
            for (std::int32_t step = from; step < to; ++step)
            {
                events.push_back(
                    TickEvent{
                        .tick = tick++,
                        .event = moveTo(300 + step * 4, 200 + step * 3)});
            }
        };

        wander(0, 10);

        events.push_back(
            TickEvent{
                .tick = tick++,
                .event = codec.encode(
                    PointerButtonPressed{
                        .button = MouseButton::Middle,
                        .position = {.x = 336, .y = 227}})});

        // Held, so every one of these moves the camera.
        for (std::int32_t step = 1; step <= 3; ++step)
        {
            events.push_back(
                TickEvent{
                    .tick = tick++,
                    .event = moveTo(336 + step * 10, 227 - step * 5)});
        }

        events.push_back(
            TickEvent{
                .tick = tick++,
                .event = codec.encode(
                    PointerButtonReleased{
                        .button = MouseButton::Middle,
                        .position = {.x = 366, .y = 212}})});

        wander(10, 15);

        events.push_back(
            TickEvent{
                .tick = tick++,
                .event = codec.encode(PointerScrolled{.vertical = -1})});

        events.push_back(
            TickEvent{
                .tick = tick + 1,
                .event = Event{.name = antwika::engine::events::kStop}});

        return events;
    }

    [[nodiscard]] std::size_t inputEventsIn(
        const std::vector<TickEvent> &events)
    {
        const InputEventCodec codec;

        std::size_t count = 0;
        for (const auto &event : events)
        {
            if (codec.decode(event.event).has_value())
            {
                ++count;
            }
        }
        return count;
    }
} // namespace

// What the gate must not cost this app: the pan or the zoom anchor.
// A dropped mid-drag movement would shorten the pan.
// A dropped last movement before the wheel would anchor a stale zoom.
TEST(ReplayDeterminismTest, TheIdleMotionGateLeavesTheCameraWhereItWas)
{
    const InputEventCodec codec;

    ReplaySource ungatedSource(wanderingSession());
    const auto ungated = run(ungatedSource);

    ReplaySource gatedSource(wanderingSession());
    IdleMotionSource gate(gatedSource, codec);
    const auto gated = run(gate);

    EXPECT_EQ(gated.summary, ungated.summary);

    // Neither run may have simply sat still.
    EXPECT_NE(ungated.summary.camera.pan(), Camera().pan());
    EXPECT_LT(
        ungated.summary.camera.zoomLevel(),
        antwika::game::kDefaultZoomLevel);
}

TEST(ReplayDeterminismTest, TheIdleMotionGateShortensTheRecording)
{
    const InputEventCodec codec;

    ReplaySource ungatedSource(wanderingSession());
    const auto ungated = run(ungatedSource);

    ReplaySource gatedSource(wanderingSession());
    IdleMotionSource gate(gatedSource, codec);
    const auto gated = run(gate);

    // Fifteen wandering movements in, and two out.
    // One is released ahead of the press, one ahead of the scroll.
    // The other thirteen went unread.
    // The press, the drag, the release and the scroll all stay.
    EXPECT_EQ(inputEventsIn(ungated.recorded), 21U);
    EXPECT_EQ(inputEventsIn(gated.recorded), 8U);
}

// A spawner is a pure function of the tick and the state.
// So the walkers it produced have to come back identically.
namespace
{
    [[nodiscard]] std::vector<TickEvent> buildingSession()
    {
        const InputEventCodec codec;
        // A source rather than a house.
        // A house consumes what is brought and sends nobody out.
        // So it would prove nothing about a regenerated spawn.
        const auto palette =
            pixelOn(antwika::game::widgets::toolWidget(
                antwika::game::BuildTool::FoodSource));

        std::vector<TickEvent> events{
            TickEvent{
                .tick = 0,
                .event = codec.encode(
                    antwika::input::PointerMoved{.position = palette})},
            TickEvent{
                .tick = 0,
                .event = codec.encode(
                    PointerButtonPressed{
                        .button = MouseButton::Left,
                        .position = palette})}};

        // A road laid with the road tool before the palette moves off.
        // It runs above both blocks rather than between them.
        // A source is two cells square.
        // It would swallow a road laid under its own footprint.
        // Kept well down the grid.
        // The toolbar covers the top, and a press it covers is dropped.
        for (std::int32_t x = 2; x <= 6; ++x)
        {
            events.insert(
                events.begin() + (x - 2),
                TickEvent{
                    .tick = 0,
                    .event =
                        pressAt(Cell{.x = x, .y = 4}, MouseButton::Left)});
        }

        // Straight after the five presses inserted above.
        // And so still ahead of the palette press it started with.
        events.insert(
            events.begin() + 5,
            TickEvent{
                .tick = 0,
                .event =
                    releaseAt(Cell{.x = 6, .y = 4}, MouseButton::Left)});

        // Two sources, placed a tick apart, so their cadences differ.
        // Two cells apart as well, so their blocks do not overlap.
        events.push_back(
            TickEvent{
                .tick = 1,
                .event = pressAt(Cell{.x = 2, .y = 5}, MouseButton::Left)});
        events.push_back(
            TickEvent{
                .tick = 2,
                .event = pressAt(Cell{.x = 5, .y = 5}, MouseButton::Left)});

        events.push_back(
            TickEvent{
                .tick = 30,
                .event = Event{.name = antwika::engine::events::kStop}});

        return events;
    }
} // namespace

namespace
{
    // A session that leaves build mode with a right click.
    // The house is selected, cancelled, and the same cell clicked again.
    // That click places nothing, the palette having been put down.
    // The road is then picked up and a second cell clicked.
    // So what the run ends with says which tool each click meant.
    [[nodiscard]] std::vector<TickEvent> cancelSession()
    {
        const InputEventCodec codec;
        const auto palette = pixelOn(
            antwika::game::widgets::toolWidget(
                antwika::game::BuildTool::House));
        const auto roadButton = pixelOn(
            antwika::game::widgets::toolWidget(
                antwika::game::BuildTool::Road));

        return {
            TickEvent{
                .tick = 0,
                .event = codec.encode(
                    antwika::input::PointerMoved{.position = palette})},
            TickEvent{
                .tick = 0,
                .event = codec.encode(
                    PointerButtonPressed{
                        .button = MouseButton::Left,
                        .position = palette})},
            TickEvent{
                .tick = 1,
                .event =
                    pressAt(Cell{.x = 3, .y = 5}, MouseButton::Right)},
            TickEvent{
                .tick = 2,
                .event = pressAt(Cell{.x = 3, .y = 5}, MouseButton::Left)},
            TickEvent{
                .tick = 3,
                .event = codec.encode(
                    antwika::input::PointerMoved{.position = roadButton})},
            TickEvent{
                .tick = 3,
                .event = codec.encode(
                    PointerButtonPressed{
                        .button = MouseButton::Left,
                        .position = roadButton})},
            TickEvent{
                .tick = 4,
                .event = pressAt(Cell{.x = 4, .y = 5}, MouseButton::Left)},
            TickEvent{
                .tick = 5,
                .event = Event{.name = antwika::engine::events::kStop}}};
    }
} // namespace

// Leaving build mode is a click and nothing else on the wire.
// So a replay has to arrive at the same palette from the same clicks.
TEST(ReplayDeterminismTest, ARightClickCancelReplaysToTheSameState)
{
    auto script = cancelSession();
    ReplaySource liveSource(script);
    const auto live = runWithToolbar(liveSource);

    // The cancel landed: the click after it placed nothing at all.
    // Not a house, and not the road a fallback would have laid.
    // The only cell holding anything is the one the road tool clicked.
    EXPECT_TRUE(live.summary.buildings.empty());
    ASSERT_EQ(live.summary.paths.size(), 1U);
    EXPECT_EQ(live.summary.paths[0], (Cell{.x = 4, .y = 5}));

    const ScratchFile file("antwika-game-cancel.replay");
    antwika::replay::saveReplayFile(live.recorded, file.name());
    auto loaded = antwika::replay::loadReplayFile(file.name());

    // Nothing about a mode may be in the file, only the clicks.
    const InputEventCodec codec;
    for (const auto &event : loaded)
    {
        EXPECT_TRUE(
            codec.decode(event.event).has_value()
            || event.event.name == antwika::engine::events::kStop)
            << event.event.name;
    }

    ReplaySource replayedSource(std::move(loaded));
    const auto replayed = runWithToolbar(replayedSource);

    EXPECT_EQ(replayed.summary, live.summary);
    EXPECT_EQ(replayed.recorded, live.recorded);
}

// Two runs that both did nothing would agree for the wrong reason.
// Without the cancel that same click puts a house up instead.
TEST(ReplayDeterminismTest, TheCancelIsWhatChangesWhatTheLastClickPlaces)
{
    auto script = cancelSession();

    // Drop the right click, and only it.
    script.erase(script.begin() + 2);

    ReplaySource source(script);
    const auto result = runWithToolbar(source);

    // The house the cancelled run refused, and the road either lays.
    ASSERT_EQ(result.summary.buildings.size(), 1U);
    EXPECT_EQ(result.summary.buildings[0].at, (Cell{.x = 3, .y = 5}));
    ASSERT_EQ(result.summary.paths.size(), 1U);
    EXPECT_EQ(result.summary.paths[0], (Cell{.x = 4, .y = 5}));
}

TEST(ReplayDeterminismTest, ABuildingsWalkersAreRegeneratedRatherThanStored)
{
    auto script = buildingSession();
    ReplaySource liveSource(script);
    const auto live = runWithToolbar(liveSource);

    // Two sources, and both of them sent somebody out.
    ASSERT_EQ(live.summary.buildings.size(), 2U);
    EXPECT_GE(live.summary.walkers.size(), 2U);

    // Nothing of the spawn is on the wire; every event is input or tick.
    for (const auto &event : live.recorded)
    {
        EXPECT_TRUE(
            event.event.name.starts_with("input.")
            || event.event.name == antwika::engine::events::kTick
            || event.event.name == antwika::engine::events::kStop);
    }

    const ScratchFile file("antwika-game-spawn.replay");
    antwika::replay::saveReplayFile(live.recorded, file.name());
    auto loaded = antwika::replay::loadReplayFile(file.name());

    ReplaySource replayedSource(std::move(loaded));
    const auto replayed = runWithToolbar(replayedSource);

    EXPECT_EQ(replayed.summary, live.summary);
}
