#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

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
#include <antwika/i18n/Translator.hpp>
#include <antwika/input/InputEvent.hpp>
#include <antwika/input/InputEventCodec.hpp>
#include <antwika/input/MouseButton.hpp>
#include <antwika/log/mocks/MockLogger.hpp>
#include <antwika/replay/ReplaySource.hpp>
#include <antwika/ui/Pointer.hpp>

#include "antwika/atlas_editor/AtlasEditor.hpp"
#include "antwika/atlas_editor/Canvas.hpp"
#include "antwika/atlas_editor/EditorState.hpp"
#include "antwika/atlas_editor/EditorUi.hpp"
#include "antwika/atlas_editor/IAtlasStore.hpp"
#include "antwika/atlas_editor/Pixel.hpp"
#include "antwika/atlas_editor/TileGrid.hpp"
#include "antwika/atlas_editor/UiOverlay.hpp"

using antwika::atlas_editor::bootstrap;
using antwika::atlas_editor::Canvas;
using antwika::atlas_editor::describeEditor;
using antwika::atlas_editor::EditorConfig;
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

    class MemoryStore final : public IAtlasStore
    {
    public:
        explicit MemoryStore(std::optional<Bitmap> opening = std::nullopt)
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

        [[nodiscard]] std::string savePath() const override
        {
            return "memory.png";
        }

        std::optional<Bitmap> available{};
        std::optional<Bitmap> written{};
    };

    // Where the save button lands, off the layout the session lays out.
    // The locale is a constant of the build, so a test may name one.
    constexpr antwika::i18n::Translator kTranslator{
        antwika::i18n::kDefaultLocale};

    // A second guess at it would be a second layout.
    // That is exactly what ui::Frame::rects exists to avoid.
    Point saveButton()
    {
        const EditorState state{
            Canvas::blank(kCanvas), TileGrid{}, kCanvas};
        const auto rect =
            describeEditor(state, Pointer{}, kTranslator)
                .rects.find(antwika::atlas_editor::widgets::kSave);

        const auto found = rect.value_or(antwika::gfx::Rect{});

        return Point{
            .x = found.origin.x
                 + static_cast<std::int32_t>(found.size.width / 2),
            .y = found.origin.y
                 + static_cast<std::int32_t>(found.size.height / 2)};
    }

    std::vector<TickEvent> script()
    {
        const InputEventCodec codec;
        const Point save = saveButton();

        return {
            TickEvent{
                .tick = 2,
                .event = codec.encode(PointerButtonPressed{
                    .button = MouseButton::Left,
                    .position = {.x = 120, .y = 300}})},
            TickEvent{
                .tick = 3,
                .event = codec.encode(PointerButtonReleased{
                    .button = MouseButton::Left,
                    .position = {.x = 120, .y = 300}})},
            TickEvent{
                .tick = 4,
                .event = codec.encode(PointerButtonPressed{
                    .button = MouseButton::Left,
                    .position = {.x = save.x, .y = save.y}})},
            TickEvent{
                .tick = 5,
                .event = codec.encode(PointerButtonReleased{
                    .button = MouseButton::Left,
                    .position = {.x = save.x, .y = save.y}})},
            TickEvent{
                .tick = 8,
                .event = Event{
                    .name = antwika::engine::events::kStop}}};
    }

    // What the watcher below saw, kept outside it.
    // bootstrap() owns the sink and destroys it before returning.
    struct WatchedTicks
    {
        std::uint64_t ticks = 0;
        std::uint64_t edits = 0;
        std::size_t commands = 0;
    };

    // Stands in for the renderer main.cpp hands bootstrap().
    // It reads the same two things RenderSink does.
    // So what it sees is what a frame would be drawn from.
    class FinishedTickWatcher final
        : public antwika::event::ITickEventSink
    {
    public:
        FinishedTickWatcher(
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
        }

    private:
        const EditorState &state;
        const UiOverlay &overlay;
        WatchedTicks &seen;
    };

    EditorSummary runOnce(MemoryStore &store)
    {
        NiceMock<MockLogger> logger;
        NiceMock<MockEventSink> eventSink;
        const InputEventCodec codec;
        ReplaySource source(script());

        return bootstrap(EditorConfig{
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
} // namespace

// The requirement this project exists for, for this app.
// A session is driven entirely by what a source hands it.
// No edit, no tool and no view is recorded -- only the clicks are.
// So running the same input twice has to land on the same sheet.
TEST(RunIntegrationTest, TheSameInputReachesTheSameSheetTwice)
{
    MemoryStore first;
    MemoryStore second;

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

TEST(RunIntegrationTest, AClickOnTheSheetPaintsAndOneOnTheBarSaves)
{
    MemoryStore store;

    const EditorSummary summary = runOnce(store);

    EXPECT_EQ(summary.edits, 1U);
    EXPECT_EQ(summary.saves, 1U);
    EXPECT_EQ(summary.image, kCanvas);
}

TEST(RunIntegrationTest, AStopEventEndsTheSessionBeforeTheCap)
{
    MemoryStore store;

    const EditorSummary summary = runOnce(store);

    EXPECT_GT(summary.ticks, 0U);
    EXPECT_LT(summary.ticks, kMaxTicks);
}

// A session opens whatever the store has.
// A blank sheet is opened only when the store has nothing.
// That is the one branch a main() is not allowed to hold.
TEST(RunIntegrationTest, ASessionOpensTheSheetTheStoreAlreadyHas)
{
    MemoryStore store(Bitmap{
        .size = {.width = 4, .height = 4},
        .pixels = std::vector<std::uint8_t>(64, 0)});

    const EditorSummary summary = runOnce(store);

    EXPECT_EQ(summary.image, (Size{.width = 4, .height = 4}));

    // The click at (120, 300) fell well outside a four by four sheet.
    EXPECT_EQ(summary.edits, 0U);
}

TEST(RunIntegrationTest, TheExtraSinkSeesEveryFinishedTick)
{
    NiceMock<MockLogger> logger;
    NiceMock<MockEventSink> eventSink;
    const InputEventCodec codec;
    ReplaySource source(script());
    MemoryStore store;

    WatchedTicks seen;
    const EditorSummary summary = bootstrap(EditorConfig{
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
            return std::make_unique<FinishedTickWatcher>(
                state, overlay, seen);
        }});

    EXPECT_EQ(seen.ticks, summary.ticks);
    EXPECT_EQ(seen.edits, summary.edits);
    EXPECT_GT(seen.commands, 0U);
}

// A caller persisting a `--record` file has no pre-known script.
// It passes an optional replayRecorder instead.
// Only the input comes back: every pixel is regenerated from it.
TEST(RunIntegrationTest, TheReplayRecorderReceivesEveryDispatchedEvent)
{
    NiceMock<MockLogger> logger;
    NiceMock<MockEventSink> eventSink;
    const InputEventCodec codec;
    ReplaySource source(script());
    TickEventRecorder recorder;
    MemoryStore store;

    const EditorSummary summary = bootstrap(EditorConfig{
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
