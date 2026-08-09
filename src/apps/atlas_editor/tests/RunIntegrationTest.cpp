#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <antwika/engine/Events.hpp>
#include <antwika/event/Event.hpp>
#include <antwika/event/ITickEventSink.hpp>
#include <antwika/event/mocks/MockEventSink.hpp>
#include <antwika/event/TickEvent.hpp>
#include <antwika/event/TickEventRecorder.hpp>
#include <antwika/gfx/Bitmap.hpp>
#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/Rect.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/i18n/Locale.hpp>
#include <antwika/input/InputEvent.hpp>
#include <antwika/input/InputEventCodec.hpp>
#include <antwika/input/Key.hpp>
#include <antwika/input/MouseButton.hpp>
#include <antwika/log/mocks/MockLogger.hpp>
#include <antwika/replay/ReplaySource.hpp>
#include <antwika/ui/Pointer.hpp>

#include "antwika/atlas_editor/AtlasEditor.hpp"
#include "antwika/atlas_editor/AtlasEditorError.hpp"
#include "antwika/atlas_editor/Canvas.hpp"
#include "antwika/atlas_editor/EditorState.hpp"
#include "antwika/atlas_editor/EditorUi.hpp"
#include "antwika/atlas_editor/FileList.hpp"
#include "antwika/atlas_editor/AtlasMeta.hpp"
#include "antwika/atlas_editor/IAtlasStore.hpp"
#include "antwika/atlas_editor/Messages.hpp"
#include "antwika/atlas_editor/Pixel.hpp"
#include "antwika/atlas_editor/TileGrid.hpp"
#include "antwika/atlas_editor/UiOverlay.hpp"

using antwika::atlas_editor::bootstrap;
using antwika::atlas_editor::Canvas;
using antwika::atlas_editor::describeEditor;
using antwika::atlas_editor::EditorWiring;
using antwika::atlas_editor::EditorState;
using antwika::atlas_editor::EditorSummary;
using antwika::atlas_editor::IAtlasStore;
using antwika::atlas_editor::Pixel;
using antwika::atlas_editor::TileGrid;
using antwika::atlas_editor::UiOverlay;
using antwika::event::Event;
using antwika::event::mocks::MockEventSink;
using antwika::event::TickEvent;
using antwika::event::TickEventRecorder;
using antwika::gfx::Bitmap;
using antwika::gfx::Point;
using antwika::gfx::Size;
using antwika::input::InputEventCodec;
using antwika::input::MouseButton;
using antwika::input::PointerButtonPressed;
using antwika::input::PointerButtonReleased;
using antwika::log::mocks::MockLogger;
using antwika::replay::ReplaySource;
using antwika::ui::Pointer;
using ::testing::NiceMock;

namespace
{
    constexpr Size kCanvas{.width = 800, .height = 480};
    constexpr antwika::time::Tick kMaxTicks = 20;

    class FakeMemoryStore final : public IAtlasStore
    {
    public:
        explicit FakeMemoryStore(std::optional<Bitmap> opening = std::nullopt)
            : available(std::move(opening))
        {
        }

        std::optional<Bitmap> load() override
        {
            return available;
        }

        void save(const Bitmap &image) override
        {
            written = image;
        }


        [[nodiscard]] std::optional<Bitmap> loadFrom(
            const std::string &path) override
        {
            opened = path;

            return load();
        }

        void saveTo(const Bitmap &image, const std::string &path)
            override
        {
            wrote = path;

            save(image);
        }

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
            return "memory.png";
        }

        std::optional<Bitmap> available{};
        std::optional<std::string> opened{};
        std::optional<std::string> wrote{};
        std::optional<Bitmap> written{};
    };

    constexpr antwika::atlas_editor::Translator kTranslator{
        antwika::i18n::kDefaultLocale};

    Point middleOf(
        const EditorState &state,
        const antwika::ui::WidgetId widget)
    {
        const auto found =
            describeEditor(state, Pointer{}, kTranslator)
                .rects.find(widget)
                .value_or(antwika::gfx::Rect{});

        return Point{
            .x = found.origin.x
                 + static_cast<std::int32_t>(found.size.width / 2),
            .y = found.origin.y
                 + static_cast<std::int32_t>(found.size.height / 2)};
    }

    Point fileMenuButton()
    {
        const EditorState state{
            Canvas::blank(kCanvas), TileGrid{}, kCanvas};

        return middleOf(
            state, antwika::atlas_editor::widgets::kFileMenu);
    }

    Point saveItem()
    {
        EditorState state{
            Canvas::blank(kCanvas), TileGrid{}, kCanvas};
        state.showMenu(antwika::atlas_editor::Menu::File);

        return middleOf(
            state,
            antwika::atlas_editor::widgets::fileItemWidget(
                antwika::atlas_editor::FileItem::Save));
    }

    Point confirmButton()
    {
        EditorState state{
            Canvas::blank(kCanvas), TileGrid{}, kCanvas};
        state.showModal(
            antwika::atlas_editor::Modal::Save,
            ".",
            antwika::atlas_editor::entriesIn("."));

        return middleOf(
            state, antwika::atlas_editor::widgets::kFileConfirm);
    }

    std::vector<TickEvent> script(bool painting = true)
    {
        const InputEventCodec codec;
        const Point menu = fileMenuButton();
        const Point save = saveItem();
        const Point confirm = confirmButton();

        std::vector<TickEvent> events;

        if (painting)
        {
            events.push_back(
                TickEvent{
                    .tick = 2,
                    .event = codec.encode(PointerButtonPressed{
                        .button = MouseButton::Left,
                        .position = {.x = 120, .y = 300}})});
            events.push_back(
                TickEvent{
                    .tick = 3,
                    .event = codec.encode(PointerButtonReleased{
                        .button = MouseButton::Left,
                        .position = {.x = 120, .y = 300}})});
        }

        events.push_back(
            TickEvent{
                .tick = 4,
                .event = codec.encode(PointerButtonPressed{
                    .button = MouseButton::Left,
                    .position = {.x = menu.x, .y = menu.y}})});
        events.push_back(
            TickEvent{
                .tick = 5,
                .event = codec.encode(PointerButtonReleased{
                    .button = MouseButton::Left,
                    .position = {.x = menu.x, .y = menu.y}})});
        events.push_back(
            TickEvent{
                .tick = 6,
                .event = codec.encode(PointerButtonPressed{
                    .button = MouseButton::Left,
                    .position = {.x = save.x, .y = save.y}})});
        events.push_back(
            TickEvent{
                .tick = 7,
                .event = codec.encode(PointerButtonReleased{
                    .button = MouseButton::Left,
                    .position = {.x = save.x, .y = save.y}})});
        events.push_back(
            TickEvent{
                .tick = 7,
                .event = codec.encode(antwika::input::KeyPressed{
                    .key = antwika::input::Key::A})});
        events.push_back(
            TickEvent{
                .tick = 7,
                .event = codec.encode(PointerButtonPressed{
                    .button = MouseButton::Left,
                    .position = {.x = confirm.x, .y = confirm.y}})});
        events.push_back(
            TickEvent{
                .tick = 7,
                .event = codec.encode(PointerButtonReleased{
                    .button = MouseButton::Left,
                    .position = {.x = confirm.x, .y = confirm.y}})});
        events.push_back(
            TickEvent{
                .tick = 8,
                .event = Event{
                    .name = antwika::engine::events::kStop}});

        return events;
    }

    struct WatchedTicks final
    {
        std::uint64_t ticks = 0;
        std::uint64_t edits = 0;
        std::size_t commands = 0;
        TileGrid tiles{};
    };

    class FakeFinishedTickWatcher final
        : public antwika::event::ITickEventSink
    {
    public:
        FakeFinishedTickWatcher(
            const EditorState &state,
            const UiOverlay &overlay,
            WatchedTicks &seen)
            : state(state), overlay(overlay), seen(seen)
        {
        }

        void handle(const TickEvent &event) override
        {
            if (event.event.name != antwika::engine::events::kTick)
            {
                return;
            }

            ++seen.ticks;
            seen.edits = state.edits();
            seen.commands = overlay.commands().size();
            seen.tiles = state.tiles();
        }

    private:
        const EditorState &state;
        const UiOverlay &overlay;
        WatchedTicks &seen;
    };

    EditorSummary runOnce(FakeMemoryStore &store, bool painting = true)
    {
        NiceMock<MockLogger> logger;
        NiceMock<MockEventSink> eventSink;
        const InputEventCodec codec;
        ReplaySource source(script(painting));

        return bootstrap(EditorWiring{
            .logger = logger,
            .eventSink = eventSink,
            .inputSource = source,
            .codec = codec,
            .store = store,
            .translator = kTranslator,
            .canvas = kCanvas,
            .blank = kCanvas,
            .tiles = TileGrid{},
            .maxTicks = kMaxTicks});
    }
}

TEST(RunIntegrationTest, RunOnce_ReachesTheSameSheetTwice)
{
    FakeMemoryStore first;
    FakeMemoryStore second;

    const EditorSummary one = runOnce(first);
    const EditorSummary two = runOnce(second);

    EXPECT_EQ(one.ticks, two.ticks);
    EXPECT_EQ(one.edits, two.edits);
    EXPECT_EQ(one.saves, two.saves);
    EXPECT_EQ(one.loads, two.loads);
    EXPECT_EQ(one.image, two.image);

    ASSERT_TRUE(first.written.has_value());
    ASSERT_TRUE(second.written.has_value());
    EXPECT_EQ(*first.written, *second.written);
}

TEST(RunIntegrationTest, RunOnce_PaintsOnTheSheetAndSavesOnTheBar)
{
    FakeMemoryStore store;
    FakeMemoryStore untouched;

    const EditorSummary summary = runOnce(store);
    const EditorSummary unpainted = runOnce(untouched, false);

    EXPECT_EQ(summary.edits, 1U);
    EXPECT_EQ(summary.saves, 1U);
    EXPECT_EQ(summary.image, kCanvas);

    EXPECT_EQ(unpainted.edits, 0U);
    EXPECT_EQ(unpainted.saves, 1U);

    ASSERT_TRUE(store.written.has_value());
    ASSERT_TRUE(untouched.written.has_value());
    EXPECT_NE(*store.written, *untouched.written);
}

TEST(RunIntegrationTest, RunOnce_EndsOnAStopBeforeTheCap)
{
    FakeMemoryStore store;

    const EditorSummary summary = runOnce(store);

    EXPECT_GT(summary.ticks, 0U);
    EXPECT_LT(summary.ticks, kMaxTicks);
}

TEST(RunIntegrationTest, RunOnce_OpensTheSheetTheStoreHas)
{
    FakeMemoryStore store(Bitmap{
        .size = {.width = 4, .height = 4},
        .pixels = std::vector<std::uint8_t>(64, 0)});

    const EditorSummary summary = runOnce(store);

    EXPECT_EQ(summary.image, (Size{.width = 4, .height = 4}));

    EXPECT_EQ(summary.edits, 0U);
}

TEST(RunIntegrationTest, Run_ShowsTheExtraSinkEveryFinishedTick)
{
    NiceMock<MockLogger> logger;
    NiceMock<MockEventSink> eventSink;
    const InputEventCodec codec;
    ReplaySource source(script());
    FakeMemoryStore store;

    WatchedTicks seen;
    const EditorSummary summary = bootstrap(EditorWiring{
        .logger = logger,
        .eventSink = eventSink,
        .inputSource = source,
        .codec = codec,
        .store = store,
        .translator = kTranslator,
        .canvas = kCanvas,
        .blank = kCanvas,
        .tiles = TileGrid{},
        .maxTicks = kMaxTicks,
        .extraSink =
            [&seen](const EditorState &state, const UiOverlay &overlay)
        {
            return std::make_unique<FakeFinishedTickWatcher>(
                state, overlay, seen);
        }});

    EXPECT_EQ(seen.ticks, summary.ticks);
    EXPECT_EQ(seen.edits, summary.edits);
    EXPECT_GT(seen.commands, 0U);
}

TEST(RunIntegrationTest, Replay_RefusesAnotherSheet)
{
    NiceMock<MockLogger> logger;
    NiceMock<MockEventSink> eventSink;
    const InputEventCodec codec;
    TickEventRecorder recorder;

    {
        ReplaySource source(script());
        FakeMemoryStore store;

        (void)bootstrap(EditorWiring{
            .logger = logger,
            .eventSink = eventSink,
            .inputSource = source,
            .codec = codec,
            .store = store,
            .translator = kTranslator,
            .canvas = kCanvas,
            .blank = kCanvas,
            .tiles = TileGrid{},
            .maxTicks = kMaxTicks,
            .announceOpening = true,
            .replayRecorder = recorder});
    }

    FakeMemoryStore changed(Bitmap{
        .size = {.width = 4, .height = 4},
        .pixels = std::vector<std::uint8_t>(64, 9)});
    ReplaySource replay(recorder.getEvents());

    EXPECT_THROW(
        (void)bootstrap(EditorWiring{
            .logger = logger,
            .eventSink = eventSink,
            .inputSource = replay,
            .codec = codec,
            .store = changed,
            .translator = kTranslator,
            .canvas = kCanvas,
            .blank = kCanvas,
            .tiles = TileGrid{},
            .maxTicks = kMaxTicks}),
        antwika::atlas_editor::AtlasEditorError);
}

TEST(RunIntegrationTest, Replay_PlaysThroughOnTheSameSheet)
{
    NiceMock<MockLogger> logger;
    NiceMock<MockEventSink> eventSink;
    const InputEventCodec codec;
    TickEventRecorder recorder;

    {
        ReplaySource source(script());
        FakeMemoryStore store;

        (void)bootstrap(EditorWiring{
            .logger = logger,
            .eventSink = eventSink,
            .inputSource = source,
            .codec = codec,
            .store = store,
            .translator = kTranslator,
            .canvas = kCanvas,
            .blank = kCanvas,
            .tiles = TileGrid{},
            .maxTicks = kMaxTicks,
            .announceOpening = true,
            .replayRecorder = recorder});
    }

    FakeMemoryStore same;
    ReplaySource replay(recorder.getEvents());

    const EditorSummary summary = bootstrap(EditorWiring{
        .logger = logger,
        .eventSink = eventSink,
        .inputSource = replay,
        .codec = codec,
        .store = same,
        .translator = kTranslator,
        .canvas = kCanvas,
        .blank = kCanvas,
        .tiles = TileGrid{},
        .maxTicks = kMaxTicks});

    EXPECT_EQ(summary.edits, 1U);
    EXPECT_EQ(summary.saves, 1U);
}

TEST(RunIntegrationTest, Run_GivesTheRecorderEveryDispatchedEvent)
{
    NiceMock<MockLogger> logger;
    NiceMock<MockEventSink> eventSink;
    const InputEventCodec codec;
    ReplaySource source(script());
    TickEventRecorder recorder;
    FakeMemoryStore store;

    const EditorSummary summary = bootstrap(EditorWiring{
        .logger = logger,
        .eventSink = eventSink,
        .inputSource = source,
        .codec = codec,
        .store = store,
        .translator = kTranslator,
        .canvas = kCanvas,
        .blank = kCanvas,
        .tiles = TileGrid{},
        .maxTicks = kMaxTicks,
        .replayRecorder = recorder});

    std::vector<TickEvent> supplied;
    for (const TickEvent &event : recorder.getEvents())
    {
        if (event.event.name != antwika::engine::events::kTick)
        {
            supplied.push_back(event);
        }
    }

    EXPECT_EQ(supplied, script());
    EXPECT_EQ(
        recorder.getEvents().size(), supplied.size() + summary.ticks);
}

TEST(RunIntegrationTest, Bootstrap_TakesOnTheAtlasTheOpenedImageCarries)
{
    NiceMock<MockLogger> logger;
    NiceMock<MockEventSink> eventSink;
    const InputEventCodec codec;
    ReplaySource source(script(false));
    FakeMemoryStore store;

    store.describes = antwika::atlas_editor::AtlasMeta{
        .kind = antwika::atlas_editor::AtlasKind::Isometric,
        .columns = 0,
        .rows = 0,
        .sprite = {.width = 32, .height = 16},
        .pivot = {.x = 16, .y = 16},
        .isometric = {.width = 16, .height = 8}};

    WatchedTicks seen;
    (void)bootstrap(EditorWiring{
        .logger = logger,
        .eventSink = eventSink,
        .inputSource = source,
        .codec = codec,
        .store = store,
        .translator = kTranslator,
        .canvas = kCanvas,
        .blank = kCanvas,
        .tiles = TileGrid{},
        .openPath = std::string("sheet.png"),
        .maxTicks = kMaxTicks,
        .extraSink =
            [&seen](const EditorState &state, const UiOverlay &overlay)
        {
            return std::make_unique<FakeFinishedTickWatcher>(
                state, overlay, seen);
        }});

    EXPECT_EQ(seen.tiles, (TileGrid{.width = 32, .height = 16}));
}

TEST(RunIntegrationTest, Bootstrap_KeepsTheAtlasWhereTheImageCarriesNone)
{
    NiceMock<MockLogger> logger;
    NiceMock<MockEventSink> eventSink;
    const InputEventCodec codec;
    ReplaySource source(script(false));
    FakeMemoryStore store;

    WatchedTicks seen;
    (void)bootstrap(EditorWiring{
        .logger = logger,
        .eventSink = eventSink,
        .inputSource = source,
        .codec = codec,
        .store = store,
        .translator = kTranslator,
        .canvas = kCanvas,
        .blank = kCanvas,
        .tiles = TileGrid{},
        .openPath = std::string("sheet.png"),
        .maxTicks = kMaxTicks,
        .extraSink =
            [&seen](const EditorState &state, const UiOverlay &overlay)
        {
            return std::make_unique<FakeFinishedTickWatcher>(
                state, overlay, seen);
        }});

    EXPECT_EQ(seen.tiles, TileGrid{});
}
