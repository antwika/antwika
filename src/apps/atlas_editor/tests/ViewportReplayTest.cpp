#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <antwika/app/WindowPointerMapping.hpp>
#include <antwika/engine/Events.hpp>
#include <antwika/event/Event.hpp>
#include <antwika/event/TickEvent.hpp>
#include <antwika/event/TickEventRecorder.hpp>
#include <antwika/event/mocks/MockEventSink.hpp>
#include <antwika/gfx/Bitmap.hpp>
#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/Rect.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/gfx/Viewport.hpp>
#include <antwika/gfx/mocks/MockRenderer.hpp>
#include <antwika/gfx/mocks/MockTexture.hpp>
#include <antwika/gfx/mocks/MockWindow.hpp>
#include <antwika/i18n/Locale.hpp>
#include <antwika/input/InputEvent.hpp>
#include <antwika/input/InputEventCodec.hpp>
#include <antwika/input/InputPipeline.hpp>
#include <antwika/input/MouseButton.hpp>
#include <antwika/input/Position.hpp>
#include <antwika/input/fakes/FakeInputBackend.hpp>
#include <antwika/log/mocks/MockLogger.hpp>
#include <antwika/replay/ReplaySource.hpp>
#include <antwika/time/fakes/FakeSleeper.hpp>

#include "antwika/atlas_editor/AtlasEditor.hpp"
#include "antwika/atlas_editor/Canvas.hpp"
#include "antwika/atlas_editor/EditorScene.hpp"
#include "antwika/atlas_editor/EditorState.hpp"
#include "antwika/atlas_editor/EditorUi.hpp"
#include "antwika/atlas_editor/IAtlasStore.hpp"
#include "antwika/atlas_editor/Messages.hpp"
#include "antwika/atlas_editor/RenderSink.hpp"
#include "antwika/atlas_editor/TileGrid.hpp"
#include "antwika/atlas_editor/Tool.hpp"
#include "antwika/atlas_editor/UiOverlay.hpp"

/**
 * @file
 * @brief What a resizable window is allowed to change, end to end.
 *
 * The editor's window is resizable and F10 fills the screen, so the
 * size it reports is not the size it was asked for and does not stay
 * put across a session.
 * The claim this file exists to hold is the one docs/resizable-windows.md
 * makes: that changes how big the picture is drawn and where, and it
 * changes nothing else -- not which pixel a press lands on, not what a
 * recording holds, and not what a replay of that recording paints.
 *
 * apps/game's ViewportReplayTest is the same assertion for the same
 * reason, and this is its counterpart for the one application whose
 * output is the art itself.
 */
namespace
{
    using antwika::app::WindowPointerMapping;
    using antwika::atlas_editor::Canvas;
    using antwika::atlas_editor::describeEditor;
    using antwika::atlas_editor::EditorScene;
    using antwika::atlas_editor::EditorState;
    using antwika::atlas_editor::IAtlasStore;
    using antwika::atlas_editor::RenderSink;
    using antwika::atlas_editor::TileGrid;
    using antwika::atlas_editor::Tool;
    using antwika::atlas_editor::UiOverlay;
    using antwika::event::TickEvent;
    using antwika::event::TickEventRecorder;
    using antwika::event::mocks::MockEventSink;
    using antwika::gfx::Bitmap;
    using antwika::gfx::Point;
    using antwika::gfx::Rect;
    using antwika::gfx::Size;
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
    using antwika::time::fakes::FakeSleeper;
    using ::testing::_;
    using ::testing::NiceMock;
    using ::testing::Return;
    using ::testing::ReturnRef;

    // The size the window is asked for, and every layout's answer.
    constexpr Size kAsked{.width = 400, .height = 240};

    // Twice as tall and rather wider: a scale of two, and pillarboxes.
    constexpr Size kBig{.width = 1000, .height = 480};

    // A different shape again, and not a whole multiple of anything.
    constexpr Size kOdd{.width = 733, .height = 391};

    // Narrower than the canvas's shape, so the width caps the scale.
    constexpr Size kNarrow{.width = 260, .height = 480};

    constexpr Size kSheet{.width = 64, .height = 48};
    constexpr TileGrid kTiles{.width = 32, .height = 24};
    constexpr antwika::time::Tick kMaxTicks = 8;

    constexpr antwika::atlas_editor::Translator kTranslator{
        antwika::i18n::kDefaultLocale};

    class NoStore final : public IAtlasStore
    {
    public:
        std::optional<Bitmap> load() override
        {
            return std::nullopt;
        }

        void save(const Bitmap &) override {}

        [[nodiscard]] std::string savePath() const override
        {
            return "nowhere.png";
        }
    };

    struct Run
    {
        std::vector<TickEvent> recorded;
        Bitmap sheet;
        std::vector<Rect> blits;
        std::uint64_t edits = 0;
    };

    // Draws the session, and keeps the sheet it drew.
    // The state bootstrap() owns dies with the call.
    // So the pixels are taken while the session is still running.
    class WatchedRenderSink final : public antwika::event::ITickEventSink
    {
    public:
        WatchedRenderSink(
            std::unique_ptr<RenderSink> inner,
            const EditorState &state,
            Bitmap &out)
            : inner(std::move(inner)), state(state), out(out)
        {
        }

        void handle(const TickEvent &event) override
        {
            inner->handle(event);
            out = state.image().bitmap();
        }

    private:
        std::unique_ptr<RenderSink> inner;
        const EditorState &state;
        Bitmap &out;
    };

    // Where a canvas point is, on a window of this size.
    // What somebody pointing at the same thing would have produced.
    [[nodiscard]] Position onWindow(const Point canvasPoint, const Size window)
    {
        const auto placed =
            antwika::gfx::viewportFor(window, kAsked).toWindow(canvasPoint);

        return Position{.x = placed.x, .y = placed.y};
    }

    [[nodiscard]] Point widgetCentre(const antwika::ui::WidgetId widget)
    {
        const EditorState fresh{Canvas::blank(kSheet), kTiles, kAsked};
        const auto rect =
            describeEditor(fresh, antwika::ui::Pointer{}, kTranslator)
                .rects.find(widget);
        const Rect found = rect.value_or(Rect{});

        return Point{
            .x = found.origin.x
                 + static_cast<std::int32_t>(found.size.width / 2),
            .y = found.origin.y
                 + static_cast<std::int32_t>(found.size.height / 2)};
    }

    // Where the sheet's own pixels are on the canvas.
    // The view opens centred at the widest zoom, which is one to one.
    [[nodiscard]] Point onSheet(const std::int32_t x, const std::int32_t y)
    {
        const EditorState fresh{Canvas::blank(kSheet), kTiles, kAsked};
        const auto pan = fresh.view().pan;

        return Point{.x = pan.x + x, .y = pan.y + y};
    }

    // A session somebody really could have performed, one round a tick.
    // A stroke, a trip to the toolbar, and a stroke in the new colour.
    // So the sheet and the bar are both hit, at every window size.
    [[nodiscard]] std::vector<std::vector<InputEvent>> session(
        const Size window)
    {
        namespace widgets = antwika::atlas_editor::widgets;

        const auto at = [window](const Point canvasPoint)
        { return onWindow(canvasPoint, window); };

        std::vector<std::vector<InputEvent>> rounds;

        rounds.push_back(
            {PointerButtonPressed{
                 .button = MouseButton::Left,
                 .position = at(onSheet(4, 30))},
             PointerMoved{.position = at(onSheet(12, 34))},
             PointerButtonReleased{
                 .button = MouseButton::Left,
                 .position = at(onSheet(12, 34))}});

        // Anchored to the canvas.
        // So this is the same swatch whatever size the window is.
        const Point swatch = widgetCentre(widgets::swatchWidget(5));
        rounds.push_back(
            {PointerMoved{.position = at(swatch)},
             PointerButtonPressed{
                 .button = MouseButton::Left, .position = at(swatch)},
             PointerButtonReleased{
                 .button = MouseButton::Left, .position = at(swatch)}});

        const Point fill = widgetCentre(widgets::toolWidget(Tool::Fill));
        rounds.push_back(
            {PointerMoved{.position = at(fill)},
             PointerButtonPressed{
                 .button = MouseButton::Left, .position = at(fill)},
             PointerButtonReleased{
                 .button = MouseButton::Left, .position = at(fill)}});

        rounds.push_back(
            {PointerButtonPressed{
                 .button = MouseButton::Left,
                 .position = at(onSheet(40, 40))},
             PointerButtonReleased{
                 .button = MouseButton::Left,
                 .position = at(onSheet(40, 40))}});

        return rounds;
    }

    // One run, at one window size, through the wiring main.cpp ships.
    // A live run reads a device and maps what it says.
    // A replay run reads the file and maps nothing, as main.cpp does.
    [[nodiscard]] Run drive(
        const Size window,
        antwika::simulation::ITickEventSource &inner,
        FakeInputBackend &device,
        const bool live)
    {
        NiceMock<MockLogger> logger;
        NiceMock<MockEventSink> eventSink;
        NiceMock<antwika::gfx::mocks::MockWindow> windowMock;
        NiceMock<antwika::gfx::mocks::MockRenderer> renderer;
        FakeSleeper sleeper;
        const EditorScene scene;
        NoStore store;
        const InputEventCodec codec;
        TickEventRecorder recorder;

        std::vector<Rect> blits;

        ON_CALL(windowMock, isOpen()).WillByDefault(Return(true));
        ON_CALL(windowMock, renderer()).WillByDefault(ReturnRef(renderer));
        ON_CALL(windowMock, size()).WillByDefault(Return(window));
        ON_CALL(windowMock, configuredSize()).WillByDefault(Return(kAsked));
        ON_CALL(renderer, createTexture(_))
            .WillByDefault(
                [](const Bitmap &)
                {
                    return std::make_unique<
                        NiceMock<antwika::gfx::mocks::MockTexture>>();
                });
        ON_CALL(renderer, drawTexture(_, _, _, _))
            .WillByDefault(
                [&blits](
                    const antwika::gfx::ITexture &,
                    Rect,
                    const Rect destination,
                    antwika::gfx::Color)
                { blits.push_back(destination); });

        const WindowPointerMapping mapping(windowMock, kAsked);

        InputPipelineOptions options{.readsDevice = live};

        // Attached only when a device is being read.
        // A file already holds canvas positions.
        // Mapping those again would map them twice.
        if (live)
        {
            options.pointerMapping = mapping;
        }

        InputPipeline input(inner, device, codec, options);

        Bitmap painted;

        const auto summary = antwika::atlas_editor::bootstrap({
            .logger = logger,
            .eventSink = eventSink,
            .inputSource = input,
            .codec = codec,
            .store = store,
            .translator = kTranslator,
            .canvas = kAsked,
            .blank = kSheet,
            .tiles = kTiles,
            .maxTicks = kMaxTicks,
            .announceOpening = live,
            .replayRecorder = recorder,
            .extraSink =
                [&](const EditorState &state, const UiOverlay &overlay)
                -> std::unique_ptr<antwika::event::ITickEventSink>
            {
                return std::make_unique<WatchedRenderSink>(
                    std::make_unique<RenderSink>(
                        windowMock,
                        scene,
                        state,
                        overlay,
                        sleeper,
                        std::chrono::milliseconds{0}),
                    state,
                    painted);
            }});

        return Run{
            .recorded = recorder.getEvents(),
            .sheet = std::move(painted),
            .blits = std::move(blits),
            .edits = summary.edits};
    }

    // The run ends on a stop the recorder writes out like any event.
    // So the file a live run leaves ends the replay of it too.
    [[nodiscard]] Run record(const Size window)
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
    // Engine::step dispatches that for itself.
    [[nodiscard]] std::vector<TickEvent> fileOf(const Run &run)
    {
        auto events = run.recorded;

        std::erase_if(
            events,
            [](const TickEvent &event)
            {
                return event.event.name == antwika::engine::events::kTick;
            });

        return events;
    }

    [[nodiscard]] Run replay(
        const std::vector<TickEvent> &file, const Size window)
    {
        ReplaySource fromFile(file);
        FakeInputBackend untouched;

        return drive(window, fromFile, untouched, false);
    }
} // namespace

// **The claim the whole thing rests on.**
// And the one that matters most here, the art being the output.
// A session is painted on a window twice the canvas's height.
// The file it produces is replayed on windows of three other shapes.
// Every one of them ends holding the same pixels.
TEST(ViewportReplayTest, AReplayRepaintsARunOnAWindowOfAnotherSize)
{
    const auto recorded = record(kBig);
    const auto file = fileOf(recorded);

    ASSERT_FALSE(file.empty());
    ASSERT_GT(recorded.edits, 0U);

    EXPECT_EQ(replay(file, kAsked).sheet, recorded.sheet);
    EXPECT_EQ(replay(file, kOdd).sheet, recorded.sheet);
    EXPECT_EQ(replay(file, kNarrow).sheet, recorded.sheet);
    EXPECT_EQ(replay(file, kBig).sheet, recorded.sheet);
}

// The other half, so the first cannot pass for the wrong reason.
// A window twice the canvas's height records what the canvas records.
// Because a recorder only ever sees canvas coordinates.
//
// A whole scale, deliberately.
// A scale of 733/400 round-trips a pixel to within one of itself.
// So what a file holds there is where the pointer was.
// Rather than always the pixel this test's arithmetic started from.
// That costs a replay nothing, since the file is what is replayed.
// And the test above is the one that says so.
TEST(ViewportReplayTest, ARecordingHoldsCanvasPositionsWhateverTheWindow)
{
    EXPECT_EQ(record(kBig).recorded, record(kAsked).recorded);
}

// And a live session paints the same sheet at a whole scale.
TEST(ViewportReplayTest, ALiveRunPaintsTheSameSheetAtAWholeScale)
{
    EXPECT_EQ(record(kBig).sheet, record(kAsked).sheet);
}

// **At a fractional scale it paints within a pixel of that instead.**
// Which is the honest limit of this rather than a defect.
//
// A window pixel maps to a canvas pixel and back to within one.
// That costs apps/game nothing, a pixel either way being one cell.
// Here one pixel *is* the unit being edited.
// So a fractional scale can put a dot on a pixel's neighbour.
// Where the same gesture at the asked-for size would have hit it.
// Below a scale of one it is coarser still.
// A window pixel then covers more than one of the sheet's.
// Zooming the *view* in is what an artist does about that.
// And that zoom is simulation state, so a replay reproduces it.
//
// None of which costs a replay anything, and this says so.
// Whatever a session at a fractional scale painted, its file repaints.
// On a window of any size at all.
// A recording holds the canvas pixel the pointer was over.
// And that is what is replayed.
TEST(ViewportReplayTest, AFractionalScalesOwnRecordingStillRepaintsIt)
{
    for (const auto window : {kOdd, kNarrow})
    {
        const auto live = record(window);
        const auto file = fileOf(live);

        ASSERT_FALSE(file.empty());

        EXPECT_EQ(replay(file, kAsked).sheet, live.sheet);
        EXPECT_EQ(replay(file, kBig).sheet, live.sheet);
        EXPECT_EQ(replay(file, window).sheet, live.sheet);
    }
}

// None of the above may pass because nothing was drawn differently.
// The picture really is bigger on the bigger window.
TEST(ViewportReplayTest, TheSheetIsDrawnLargerInALargerWindow)
{
    const auto asked = record(kAsked);
    const auto big = record(kBig);

    ASSERT_FALSE(asked.blits.empty());
    ASSERT_EQ(asked.blits.size(), big.blits.size());

    // The same sheet at twice the size, the window being twice as tall.
    EXPECT_EQ(big.blits.front().size.width, asked.blits.front().size.width * 2);
    EXPECT_EQ(
        big.blits.front().size.height, asked.blits.front().size.height * 2);
}
