#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

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
#include <antwika/console/ConsolePicture.hpp>
#include <antwika/ui/support/WidgetCentre.hpp>

#include "antwika/atlas_editor/AtlasEditor.hpp"
#include "antwika/atlas_editor/Canvas.hpp"
#include "antwika/atlas_editor/EditorScene.hpp"
#include "antwika/atlas_editor/EditorState.hpp"
#include "antwika/atlas_editor/EditorUi.hpp"
#include "antwika/atlas_editor/AtlasMeta.hpp"
#include "antwika/atlas_editor/IAtlasStore.hpp"
#include "antwika/atlas_editor/Messages.hpp"
#include "antwika/atlas_editor/RenderSink.hpp"
#include "antwika/atlas_editor/TileGrid.hpp"
#include "antwika/atlas_editor/Tool.hpp"
#include "antwika/atlas_editor/UiOverlay.hpp"

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

    constexpr Size kAsked{.width = 400, .height = 240};

    constexpr Size kBig{.width = 1000, .height = 480};

    constexpr Size kOdd{.width = 733, .height = 391};

    constexpr Size kNarrow{.width = 260, .height = 480};

    constexpr Size kSheet{.width = 64, .height = 48};
    constexpr TileGrid kTiles{.width = 32, .height = 24};
    constexpr antwika::time::Tick kMaxTicks = 8;

    constexpr antwika::atlas_editor::Translator kTranslator{
        antwika::i18n::kDefaultLocale};

    class FakeNoStore final : public IAtlasStore
    {
    public:
        std::optional<Bitmap> load() override
        {
            return std::nullopt;
        }

        void save(const Bitmap &) override {}

        [[nodiscard]] std::optional<Bitmap> loadFrom(
            const std::string &) override
        {
            return std::nullopt;
        }

        void saveTo(const Bitmap &, const std::string &) override {}

        std::optional<antwika::atlas_editor::AtlasMeta> describes{};
        std::optional<antwika::atlas_editor::AtlasMeta> metaWritten{};
        std::optional<std::string> metaWrote{};

        [[nodiscard]] std::optional<antwika::atlas_editor::AtlasMeta>
        loadMetaFrom(const std::string &) override
        {
            return describes;
        }

        void saveMetaTo(
            const antwika::atlas_editor::AtlasMeta &meta,
            const std::string &path) override
        {
            metaWrote = path;
            metaWritten = meta;
        }

        [[nodiscard]] std::string savePath() const override
        {
            return "nowhere.png";
        }
    };

    struct Run final
    {
        std::vector<TickEvent> recorded;
        Bitmap sheet;
        std::vector<Rect> blits;
        std::uint64_t edits = 0;
    };

    class FakeWatchedRenderSink final : public antwika::event::ITickEventSink
    {
    public:
        FakeWatchedRenderSink(
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

    [[nodiscard]] Position onWindow(const Point canvasPoint, const Size window)
    {
        const auto placed =
            antwika::gfx::viewportFor(window, kAsked).toWindow(canvasPoint);

        return Position{.x = placed.x, .y = placed.y};
    }

    [[nodiscard]] Point widgetCentre(const antwika::ui::WidgetId widget)
    {
        const EditorState fresh{Canvas::blank(kSheet), kTiles, kAsked};

        return antwika::ui::support::widgetCentre(
                   describeEditor(
                       fresh, antwika::ui::Pointer{}, kTranslator),
                   widget)
            .value_or(Point{});
    }

    [[nodiscard]] Point onSheet(const std::int32_t x, const std::int32_t y)
    {
        const EditorState fresh{Canvas::blank(kSheet), kTiles, kAsked};
        const auto pan = fresh.view().pan;

        return Point{.x = pan.x + x, .y = pan.y + y};
    }

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

    [[nodiscard]] Run drive(
        const Size window,
        antwika::event::ITickEventSource &inner,
        FakeInputBackend &device,
        const bool live)
    {
        NiceMock<MockLogger> logger;
        NiceMock<MockEventSink> eventSink;
        NiceMock<antwika::gfx::mocks::MockWindow> windowMock;
        NiceMock<antwika::gfx::mocks::MockRenderer> renderer;
        FakeSleeper sleeper;
        const EditorScene scene;
        FakeNoStore store;
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

        if (live)
        {
            options.pointerMapping = mapping;
        }

        InputPipeline input(inner, device, codec, options);

        Bitmap painted;

        antwika::console::ConsolePicture consolePicture;

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
                return std::make_unique<FakeWatchedRenderSink>(
                    std::make_unique<RenderSink>(
                        windowMock,
                        scene,
                        state,
                        overlay,
                        consolePicture,
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
}

TEST(ViewportReplayTest, Replay_RepaintsOnAWindowOfAnotherSize)
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

TEST(ViewportReplayTest, Record_HoldsCanvasPositionsAnyWindow)
{
    const auto big = record(kBig);
    const auto asked = record(kAsked);

    ASSERT_FALSE(fileOf(big).empty());

    EXPECT_EQ(big.recorded, asked.recorded);
}

TEST(ViewportReplayTest, Record_PaintsTheSameSheetAtAWholeScale)
{
    const auto big = record(kBig);
    const auto asked = record(kAsked);

    const Canvas untouched = Canvas::blank(kSheet);

    ASSERT_GT(asked.edits, 0U);
    ASSERT_NE(asked.sheet, untouched.bitmap());

    EXPECT_EQ(big.sheet, asked.sheet);
}

TEST(ViewportReplayTest, Replay_RepaintsAFractionalScalesRecording)
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

TEST(ViewportReplayTest, Record_DrawsTheSheetLargerInALargerWindow)
{
    const auto asked = record(kAsked);
    const auto big = record(kBig);

    ASSERT_FALSE(asked.blits.empty());
    ASSERT_EQ(asked.blits.size(), big.blits.size());

    EXPECT_EQ(big.blits.front().size.width, asked.blits.front().size.width * 2);
    EXPECT_EQ(
        big.blits.front().size.height, asked.blits.front().size.height * 2);
}
