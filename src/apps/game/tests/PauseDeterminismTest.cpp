#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstdint>
#include <filesystem>
#include <string>
#include <system_error>
#include <vector>

#include <antwika/engine/Events.hpp>
#include <antwika/event/Event.hpp>
#include <antwika/event/TickEvent.hpp>
#include <antwika/event/TickEventRecorder.hpp>
#include <antwika/event/mocks/MockEventSink.hpp>
#include <antwika/input/InputEvent.hpp>
#include <antwika/input/InputEventCodec.hpp>
#include <antwika/input/MouseButton.hpp>
#include <antwika/input/Position.hpp>
#include <antwika/log/mocks/MockLogger.hpp>
#include <antwika/replay/ReplayCli.hpp>
#include <antwika/replay/ReplaySource.hpp>
#include <antwika/ui/Pointer.hpp>
#include <antwika/ui/WidgetId.hpp>

#include "WidgetPixel.hpp"

#include "antwika/game/AppMode.hpp"
#include "antwika/game/BuildingIndex.hpp"
#include "antwika/game/Camera.hpp"
#include "antwika/game/Cell.hpp"
#include "antwika/game/Game.hpp"
#include "antwika/game/GridExtent.hpp"
#include "antwika/game/IsoProjection.hpp"
#include "antwika/game/PathIndex.hpp"
#include "antwika/game/Toolbar.hpp"
#include "antwika/game/UiCanvas.hpp"
#include "antwika/game/UiOverlay.hpp"

using antwika::event::Event;
using antwika::event::TickEvent;
using antwika::event::TickEventRecorder;
using antwika::event::mocks::MockEventSink;
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
using antwika::game::tests::widgetCentre;
using antwika::input::InputEventCodec;
using antwika::input::MouseButton;
using antwika::input::PointerButtonPressed;
using antwika::input::Position;
using antwika::log::mocks::MockLogger;
using antwika::replay::ReplaySource;
using ::testing::NiceMock;

namespace
{
    constexpr GridExtent kExtent{.width = 16, .height = 16};
    constexpr antwika::time::Tick kMaxTicks = 40;

    // Well down the grid, since the toolbar covers the top.
    // A press the bar covers never reaches the world.
    constexpr std::int32_t kRoadRow = 6;

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

    [[nodiscard]] RunResult run(antwika::simulation::ITickSource &source)
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
        UiOverlay overlay(kUiCanvas);

        auto summary =
            antwika::game::bootstrap(antwika::game::GameConfig{
                .logger = logger,
                .eventSink = eventSink,
                .inputSource = source,
                .codec = codec,
                .extent = kExtent,
                .camera = camera,
                .paths = paths,
                .built = built,
                .mode = mode,
                .maxTicks = kMaxTicks,
                .replayRecorder = recorder,
                .overlay = overlay});

        return RunResult{
            .summary = std::move(summary),
            .recorded = recorder.getEvents()};
    }

    // Where a button is, is the layout's business.
    // So a test asks the layout for the one it means.
    // Off the arguments the run describes the bar with at first.
    [[nodiscard]] Position pixelOn(antwika::ui::WidgetId id)
    {
        const Toolbar toolbar;
        const Camera camera;
        const auto centre = widgetCentre(
            toolbar.describe(kUiCanvas, antwika::ui::Pointer{}, camera),
            id);

        if (!centre.has_value())
        {
            return Position{};
        }

        return Position{.x = centre->x, .y = centre->y};
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

    // A corridor, and a walker on it.
    // Optionally, the pause button pressed once it is under way.
    [[nodiscard]] std::vector<TickEvent> session(
        bool pausing, antwika::time::Tick stopAt = 20)
    {
        std::vector<TickEvent> events;

        for (std::int32_t x = 2; x <= 8; ++x)
        {
            events.push_back(
                TickEvent{
                    .tick = 0,
                    .event = pressAt(
                        Cell{.x = x, .y = kRoadRow}, MouseButton::Left)});
        }

        events.push_back(
            TickEvent{
                .tick = 1,
                .event = pressAt(
                    Cell{.x = 2, .y = kRoadRow}, MouseButton::Right)});

        if (pausing)
        {
            const InputEventCodec codec;

            events.push_back(
                TickEvent{
                    .tick = 4,
                    .event = codec.encode(
                        PointerButtonPressed{
                            .button = MouseButton::Left,
                            .position = pixelOn(
                                antwika::game::widgets::kPauseResume)})});
        }

        events.push_back(
            TickEvent{
                .tick = stopAt,
                .event = Event{.name = antwika::engine::events::kStop}});

        return events;
    }
} // namespace

// Two runs that both stood still would agree for the wrong reason.
TEST(PauseDeterminismTest, TheUnpausedRunActuallyWalksSomewhere)
{
    auto script = session(false);
    ReplaySource source(script);

    const auto result = run(source);

    EXPECT_EQ(result.summary.paths.size(), 7U);
    ASSERT_EQ(result.summary.walkers.size(), 1U);
    EXPECT_NE(
        result.summary.walkers[0].at, (Cell{.x = 2, .y = kRoadRow}));
}

// The button reaches the simulation rather than the picture.
// Twenty more ticks of a paused run change nothing about it.
TEST(PauseDeterminismTest, PressingPauseHoldsTheWalkersWhereTheyWere)
{
    auto shortScript = session(true, 20);
    ReplaySource shortSource(shortScript);
    const auto shortRun = run(shortSource);

    auto longScript = session(true, 39);
    ReplaySource longSource(longScript);
    const auto longRun = run(longSource);

    ASSERT_EQ(shortRun.summary.walkers.size(), 1U);
    EXPECT_EQ(shortRun.summary.walkers, longRun.summary.walkers);
}

// The same two runs without the press.
// So the equality above cannot pass on a walker that never moved.
TEST(PauseDeterminismTest, ARunningWalkerIsSomewhereElseTwentyTicksLater)
{
    auto shortScript = session(false, 20);
    ReplaySource shortSource(shortScript);
    const auto shortRun = run(shortSource);

    auto longScript = session(false, 39);
    ReplaySource longSource(longScript);
    const auto longRun = run(longSource);

    ASSERT_EQ(shortRun.summary.walkers.size(), 1U);
    EXPECT_NE(shortRun.summary.walkers, longRun.summary.walkers);
}

// What a pause does not stop: the camera, and building on the grid.
// It is a build pause rather than a freeze -- see PauseGatedSystem.
TEST(PauseDeterminismTest, APausedRunCanStillBeBuiltOnAndPannedOver)
{
    auto script = session(true);

    // Both after the pause, so both are asked of a held-still city.
    script.insert(
        script.end() - 1,
        TickEvent{
            .tick = 6,
            .event = pressAt(
                Cell{.x = 9, .y = kRoadRow}, MouseButton::Left)});

    const InputEventCodec codec;
    script.insert(
        script.end() - 1,
        TickEvent{
            .tick = 7,
            .event = codec.encode(
                antwika::input::PointerScrolled{.vertical = -1})});

    ReplaySource source(script);
    const auto result = run(source);

    EXPECT_EQ(result.summary.paths.size(), 8U);
    EXPECT_LT(
        result.summary.camera.zoomLevel(),
        antwika::game::kDefaultZoomLevel);
}

// The rule the whole design rests on.
// The pause is regenerated from the click rather than recorded.
// So a replay pauses on precisely the ticks the live run did.
TEST(PauseDeterminismTest, APausedRunReplaysToTheSameState)
{
    auto script = session(true);
    ReplaySource liveSource(script);
    const auto live = run(liveSource);

    const ScratchFile file("antwika-game-pause.replay");
    antwika::replay::saveReplayFile(live.recorded, file.name());
    auto loaded = antwika::replay::loadReplayFile(file.name());

    // Nothing about a pause may be in the file.
    const InputEventCodec codec;
    for (const auto &event : loaded)
    {
        EXPECT_EQ(event.event.name.rfind("ui.", 0), std::string::npos)
            << event.event.name;
        EXPECT_EQ(event.event.name.rfind("game.pause", 0), std::string::npos)
            << event.event.name;
        EXPECT_TRUE(
            codec.decode(event.event).has_value()
            || event.event.name == antwika::engine::events::kStop)
            << event.event.name;
    }

    ReplaySource replayedSource(std::move(loaded));
    const auto replayed = run(replayedSource);

    EXPECT_EQ(replayed.summary, live.summary);
    EXPECT_EQ(replayed.recorded, live.recorded);
}
