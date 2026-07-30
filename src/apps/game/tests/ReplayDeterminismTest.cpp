#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <filesystem>
#include <string>
#include <system_error>
#include <vector>

#include <antwika/engine/Events.hpp>
#include <antwika/event/Event.hpp>
#include <antwika/event/EventRecorder.hpp>
#include <antwika/event/TickEvent.hpp>
#include <antwika/event/TickEventRecorder.hpp>
#include <antwika/gfx/NullBackend.hpp>
#include <antwika/gfx/WindowDesc.hpp>
#include <antwika/gfx/WindowEvent.hpp>
#include <antwika/gfx/WindowId.hpp>
#include <antwika/gfx/mocks/MockGfxBackend.hpp>
#include <antwika/input/InputEvent.hpp>
#include <antwika/input/InputEventCodec.hpp>
#include <antwika/input/MouseButton.hpp>
#include <antwika/log/mocks/MockLogger.hpp>
#include <antwika/replay/ReplayCli.hpp>
#include <antwika/replay/ReplaySource.hpp>

#include "antwika/game/Camera.hpp"
#include "antwika/game/Cell.hpp"
#include "antwika/game/Events.hpp"
#include "antwika/game/Game.hpp"
#include "antwika/game/GridExtent.hpp"
#include "antwika/game/IsoProjection.hpp"
#include "antwika/game/PathIndex.hpp"
#include "antwika/game/WindowInputSource.hpp"

using antwika::event::Event;
using antwika::event::EventRecorder;
using antwika::event::TickEvent;
using antwika::event::TickEventRecorder;
using antwika::game::Camera;
using antwika::game::Cell;
using antwika::game::cellCentre;
using antwika::game::GameSummary;
using antwika::game::GridExtent;
using antwika::game::PathIndex;
using antwika::game::WindowInputSource;
using antwika::gfx::CloseRequested;
using antwika::gfx::NullBackend;
using antwika::gfx::WindowEvent;
using antwika::gfx::WindowId;
using antwika::gfx::mocks::MockGfxBackend;
using antwika::input::InputEventCodec;
using antwika::input::MouseButton;
using antwika::input::PointerButtonPressed;
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

    constexpr std::array<std::string_view, 1> kSelfGeneratedEventNames{
        antwika::game::events::kStarted,
    };

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

    [[nodiscard]] RunResult run(antwika::replay::IReplaySource &source)
    {
        NiceMock<MockLogger> logger;
        EventRecorder eventSink;
        const InputEventCodec codec;
        Camera camera;
        PathIndex paths;
        TickEventRecorder recorder;

        auto summary = antwika::game::bootstrap(
            logger,
            eventSink,
            source,
            codec,
            kExtent,
            camera,
            paths,
            {},
            kMaxTicks,
            &recorder);

        return RunResult{
            .summary = std::move(summary),
            .recorded = recorder.getEvents()};
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
        live.recorded, file.name(), kSelfGeneratedEventNames);
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
        result.recorded, file.name(), kSelfGeneratedEventNames);
    const auto loaded = antwika::replay::loadReplayFile(file.name());

    const InputEventCodec codec;
    std::size_t inputEvents = 0;
    for (const auto &event : loaded)
    {
        EXPECT_NE(event.event.name, antwika::engine::events::kTick);
        EXPECT_NE(event.event.name, antwika::game::events::kStarted);

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
        live.recorded, file.name(), kSelfGeneratedEventNames);
    auto loaded = antwika::replay::loadReplayFile(file.name());

    // Replayed under a source with no window at all.
    ReplaySource replayedSource(std::move(loaded));
    const auto replayed = run(replayedSource);

    EXPECT_EQ(replayed.summary, live.summary);
}
