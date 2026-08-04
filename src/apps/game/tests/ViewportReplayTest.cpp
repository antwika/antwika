#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <vector>

#include <antwika/app/WindowPointerMapping.hpp>
#include <antwika/engine/Events.hpp>
#include <antwika/event/Event.hpp>
#include <antwika/event/TickEvent.hpp>
#include <antwika/event/TickEventRecorder.hpp>
#include <antwika/event/mocks/MockEventSink.hpp>
#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/Rect.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/gfx/Viewport.hpp>
#include <antwika/gfx/mocks/MockRenderer.hpp>
#include <antwika/gfx/mocks/MockTexture.hpp>
#include <antwika/gfx/mocks/MockWindow.hpp>
#include <antwika/input/InputEvent.hpp>
#include <antwika/input/InputEventCodec.hpp>
#include <antwika/input/InputPipeline.hpp>
#include <antwika/input/MouseButton.hpp>
#include <antwika/input/Position.hpp>
#include <antwika/input/fakes/FakeInputBackend.hpp>
#include <antwika/log/mocks/MockLogger.hpp>
#include <antwika/replay/ReplaySource.hpp>

#include "TestTranslator.hpp"
#include "WidgetPixel.hpp"
#include "antwika/game/AppMode.hpp"
#include "antwika/game/BuildingIndex.hpp"
#include "antwika/game/Camera.hpp"
#include "antwika/game/Cell.hpp"
#include "antwika/game/Game.hpp"
#include "antwika/game/GridExtent.hpp"
#include "antwika/game/GridScene.hpp"
#include "antwika/game/IsoProjection.hpp"
#include "antwika/game/MainMenuScene.hpp"
#include "antwika/game/PathIndex.hpp"
#include "antwika/game/PauseState.hpp"
#include "antwika/game/RenderSystem.hpp"
#include "antwika/game/SaveLoadScene.hpp"
#include "antwika/game/Toolbar.hpp"
#include "antwika/game/UiCanvas.hpp"
#include "antwika/game/UiOverlay.hpp"
#include "antwika/game/WorldMap.hpp"
#include "antwika/game/WorldMapScene.hpp"
#include "antwika/game/WorldMapState.hpp"

using antwika::game::tests::kTranslator;
using antwika::game::tests::widgetCentre;

using antwika::app::WindowPointerMapping;
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
using antwika::game::GridScene;
using antwika::game::kUiCanvas;
using antwika::game::MainMenuScene;
using antwika::game::PathIndex;
using antwika::game::RenderSetup;
using antwika::game::RenderSystem;
using antwika::game::SaveLoadScene;
using antwika::game::Toolbar;
using antwika::game::UiOverlay;
using antwika::game::WorldMapScene;
using antwika::game::WorldMapState;
using antwika::gfx::Rect;
using antwika::gfx::Size;
using antwika::gfx::mocks::MockRenderer;
using antwika::gfx::mocks::MockTexture;
using antwika::gfx::mocks::MockWindow;
using antwika::input::InputEvent;
using antwika::input::InputEventCodec;
using antwika::input::InputPipeline;
using antwika::input::InputPipelineOptions;
using antwika::input::MouseButton;
using antwika::input::PointerButtonPressed;
using antwika::input::PointerButtonReleased;
using antwika::input::PointerMoved;
using antwika::input::Position;
using antwika::input::fakes::FakeInputBackend;
using antwika::log::mocks::MockLogger;
using antwika::replay::ReplaySource;
using ::testing::_;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::ReturnRef;

namespace
{
    constexpr GridExtent kExtent{.width = 16, .height = 16};
    constexpr antwika::time::Tick kMaxTicks = 12;

    // The size the window was asked for, and every layout's answer.
    constexpr Size kAsked = kUiCanvas;

    // Twice as tall and rather wider: a scale of two, and pillarboxes.
    constexpr Size kBig{.width = 2400, .height = 1280};

    // A different shape again, and not a whole multiple of anything.
    constexpr Size kOdd{.width = 1600, .height = 900};

    // Narrower than the canvas's own shape, so the width caps it.
    constexpr Size kNarrow{.width = 700, .height = 900};

    struct Run
    {
        GameSummary summary;
        std::vector<TickEvent> recorded;
        std::vector<Rect> blits;
    };

    // Where a canvas point is, on a window of this size.
    // What a user pointing at the same thing would have produced.
    [[nodiscard]] Position onWindow(
        antwika::gfx::Point canvasPoint, Size window)
    {
        const auto placed =
            antwika::gfx::viewportFor(window, kUiCanvas).toWindow(
                canvasPoint);

        return Position{.x = placed.x, .y = placed.y};
    }

    [[nodiscard]] antwika::gfx::Point zoomInButton()
    {
        const Toolbar toolbar{kTranslator};
        const Camera camera;

        const auto centre = widgetCentre(
            toolbar.describe(kUiCanvas, antwika::ui::Pointer{}, camera),
            antwika::game::widgets::kZoomIn);

        return centre.value_or(antwika::gfx::Point{});
    }

    // A session a user really could have performed, one round per tick.
    // Road laid with a drag, a walker dropped, and a press on the bar.
    // So both hit paths are exercised.
    [[nodiscard]] std::vector<std::vector<InputEvent>> session(Size window)
    {
        const auto atCell = [window](Cell cell)
        {
            return onWindow(cellCentre(cell, Camera()), window);
        };

        std::vector<std::vector<InputEvent>> rounds;

        std::vector<InputEvent> laying;
        for (std::int32_t x = 1; x <= 5; ++x)
        {
            laying.push_back(
                PointerButtonPressed{
                    .button = MouseButton::Left,
                    .position = atCell(Cell{.x = x, .y = 2})});
        }
        laying.push_back(
            PointerButtonReleased{
                .button = MouseButton::Left,
                .position = atCell(Cell{.x = 5, .y = 2})});
        rounds.push_back(std::move(laying));

        rounds.push_back(
            {PointerButtonPressed{
                .button = MouseButton::Right,
                .position = atCell(Cell{.x = 1, .y = 2})}});

        // The bar is anchored to the canvas.
        // So this is the same button whatever the window is.
        rounds.push_back(
            {PointerMoved{.position = onWindow(zoomInButton(), window)},
             PointerButtonPressed{
                 .button = MouseButton::Left,
                 .position = onWindow(zoomInButton(), window)}});

        return rounds;
    }

    // One run, at one window size, through the shipped wiring.
    // A live run reads a device and maps what it says.
    // A replay run reads the file and maps nothing, as main.cpp does.
    [[nodiscard]] Run drive(
        Size window,
        antwika::simulation::ITickEventSource &inner,
        FakeInputBackend &device,
        bool live)
    {
        NiceMock<MockLogger> logger;
        NiceMock<MockEventSink> eventSink;
        const InputEventCodec codec;
        TickEventRecorder recorder;

        Camera camera;
        PathIndex paths;
        antwika::game::BuildingIndex built;
        AppModeState mode{AppMode::CityMap};
        antwika::game::PauseState pause;
        UiOverlay overlay(kUiCanvas);
        UiOverlay menuOverlay(kUiCanvas);
        UiOverlay saveOverlay(kUiCanvas);

        NiceMock<MockRenderer> renderer;
        NiceMock<MockWindow> windowMock;
        NiceMock<MockTexture> atlas;
        ON_CALL(windowMock, renderer()).WillByDefault(ReturnRef(renderer));
        ON_CALL(windowMock, size()).WillByDefault(Return(window));
        ON_CALL(windowMock, configuredSize()).WillByDefault(Return(kAsked));

        std::vector<Rect> blits;
        ON_CALL(renderer, drawTexture(_, _, _, _))
            .WillByDefault(
                [&blits](
                    const antwika::gfx::ITexture &,
                    Rect,
                    Rect destination,
                    antwika::gfx::Color)
                { blits.push_back(destination); });

        const GridScene scene{kTranslator};
        const MainMenuScene menuScene{kTranslator};
        const SaveLoadScene saveScene{kTranslator};
        const WorldMapScene worldScene;
        antwika::input::PointerHintChannel hint;
        WorldMapState cities{
            antwika::game::generateWorldMap(
                antwika::game::WorldMapConfig{
                    .width = 6, .height = 6, .seed = 1})};

        RenderSystem renderSystem(
            RenderSetup{
                .window = windowMock,
                .mode = mode,
                .canvas = kUiCanvas,
                .scene = scene,
                .atlases =
                    {.oneByOne = atlas,
                     .twoByTwo = atlas,
                     .threeByThree = atlas},
                .paths = paths,
                .built = built,
                .camera = camera,
                .extent = kExtent,
                .pause = pause,
                .overlay = overlay,
                .hint = hint,
                .menuScene = menuScene,
                .menuOverlay = menuOverlay,
                .saveScene = saveScene,
                .saveOverlay = saveOverlay,
                .worldScene = worldScene,
                .cities = cities});

        const WindowPointerMapping mapping(windowMock, kUiCanvas);

        InputPipelineOptions options{
            .readsDevice = live,
            .coalescePointerMotion = true,
            .thinIdleMotion = true,
            .pointerHint = hint};

        if (live)
        {
            options.pointerMapping = mapping;
        }

        InputPipeline input(inner, device, codec, options);

        auto summary = antwika::game::bootstrap(
            antwika::game::GameWiring{
                .logger = logger,
                .eventSink = eventSink,
                .inputSource = input,
                .codec = codec,
                .extent = kExtent,
                .camera = camera,
                .paths = paths,
                .built = built,
                .mode = mode,
                .pause = pause,
                .observers = {renderSystem},
                .maxTicks = kMaxTicks,
                .replayRecorder = recorder,
                .overlay = overlay,
                .menuOverlay = menuOverlay,
                .world = cities,
                .saveOverlay = saveOverlay});

        return Run{
            .summary = std::move(summary),
            .recorded = recorder.getEvents(),
            .blits = std::move(blits)};
    }

    // The run ends on a stop the recorder writes out like any event.
    // So the file a live run leaves ends the replay of it too.
    [[nodiscard]] Run record(Size window)
    {
        ReplaySource ending(
            {TickEvent{
                .tick = 6,
                .event = antwika::event::Event{
                    .name = antwika::engine::events::kStop}}});
        FakeInputBackend device(session(window));

        return drive(window, ending, device, true);
    }

    // What a --record file holds is everything but engine.tick.
    // Engine::step dispatches that for itself -- see saveReplayFile().
    [[nodiscard]] std::vector<TickEvent> fileOf(const Run &run)
    {
        auto events = run.recorded;

        std::erase_if(
            events,
            [](const TickEvent &event)
            {
                return event.event.name
                       == antwika::engine::events::kTick;
            });

        return events;
    }

    [[nodiscard]] Run replay(
        const std::vector<TickEvent> &file, Size window)
    {
        ReplaySource fromFile(file);
        FakeInputBackend untouched;

        return drive(window, fromFile, untouched, false);
    }
} // namespace

// **The claim the whole viewport design rests on.**
// A session is recorded on a window twice the canvas's height.
// The file it produces is replayed on a window of another shape.
// Both reach the same city, the same walkers and the same camera.
TEST(ViewportReplayTest, AReplayReproducesARunOnAWindowOfAnotherSize)
{
    const auto recorded = record(kBig);
    const auto file = fileOf(recorded);

    ASSERT_FALSE(file.empty());

    EXPECT_EQ(replay(file, kOdd).summary, recorded.summary);
    EXPECT_EQ(replay(file, kAsked).summary, recorded.summary);
    EXPECT_EQ(replay(file, kNarrow).summary, recorded.summary);
}

// The other half, so the first cannot pass for the wrong reason.
// A window twice the canvas's height records what the canvas records.
// Because a recorder only ever sees canvas coordinates.
//
// A whole scale, deliberately.
// A scale of 45/32 round-trips a pixel to within one rather than onto itself.
// So what a file holds there is where the pointer was.
// Rather than always the pixel this test's arithmetic started from.
// That costs a replay nothing, since the file is what is replayed.
// And the test below is the one that says so.
TEST(ViewportReplayTest, ARecordingHoldsCanvasPositionsWhateverTheWindow)
{
    EXPECT_EQ(record(kBig).recorded, record(kAsked).recorded);
}

// And a live run reaches the same city whatever window it was played in.
// Including the two whose scale is not a whole number.
TEST(ViewportReplayTest, ALiveRunIsTheSameGameWhateverTheWindow)
{
    EXPECT_EQ(record(kBig).summary, record(kAsked).summary);
    EXPECT_EQ(record(kOdd).summary, record(kAsked).summary);
    EXPECT_EQ(record(kNarrow).summary, record(kAsked).summary);
}

// Neither of the above may pass because nothing was drawn differently.
// The picture really is bigger on the bigger window.
TEST(ViewportReplayTest, ThePictureScalesWithTheWindowItIsDrawnIn)
{
    const auto asked = record(kAsked);
    const auto big = record(kBig);

    ASSERT_FALSE(asked.blits.empty());
    ASSERT_EQ(asked.blits.size(), big.blits.size());

    // The same tiles at twice the size, since the window is twice as tall.
    // Nothing about which tile that is changed.
    EXPECT_EQ(big.blits.front().size.width, asked.blits.front().size.width * 2);
    EXPECT_EQ(
        big.blits.front().size.height, asked.blits.front().size.height * 2);
}
