#include <gtest/gtest.h>

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include <antwika/engine/Events.hpp>
#include <antwika/event/Event.hpp>
#include <antwika/event/TickEvent.hpp>
#include <antwika/gfx/Bitmap.hpp>
#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/GfxError.hpp>
#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/Rect.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/i18n/Locale.hpp>
#include <antwika/i18n/MessageId.hpp>
#include <antwika/i18n/Translator.hpp>
#include <antwika/input/InputEvent.hpp>
#include <antwika/input/InputEventCodec.hpp>
#include <antwika/input/Key.hpp>
#include <antwika/input/MouseButton.hpp>
#include <antwika/ui/Pointer.hpp>

#include "antwika/atlas_editor/AtlasEditorError.hpp"
#include "antwika/atlas_editor/Canvas.hpp"
#include "antwika/atlas_editor/CanvasView.hpp"
#include "antwika/atlas_editor/EditorSink.hpp"
#include "antwika/atlas_editor/EditorState.hpp"
#include "antwika/atlas_editor/EditorUi.hpp"
#include "antwika/atlas_editor/IAtlasStore.hpp"
#include "antwika/atlas_editor/Palette.hpp"
#include "antwika/atlas_editor/Pixel.hpp"
#include "antwika/atlas_editor/TileGrid.hpp"
#include "antwika/atlas_editor/Tool.hpp"
#include "antwika/atlas_editor/UiOverlay.hpp"

using antwika::atlas_editor::Canvas;
using antwika::atlas_editor::defaultPalette;
using antwika::atlas_editor::describeEditor;
using antwika::i18n::MessageId;
using antwika::atlas_editor::EditorSink;
using antwika::atlas_editor::EditorState;
using antwika::atlas_editor::IAtlasStore;
using antwika::atlas_editor::Pixel;
using antwika::atlas_editor::scaleOf;
using antwika::atlas_editor::TileGrid;
using antwika::atlas_editor::Tool;
using antwika::atlas_editor::UiOverlay;
using antwika::event::Event;
using antwika::event::TickEvent;
using antwika::gfx::Bitmap;
using antwika::gfx::Color;
using antwika::gfx::Point;
using antwika::gfx::Rect;
using antwika::gfx::Size;
using antwika::input::InputEvent;
using antwika::input::InputEventCodec;
using antwika::input::Key;
using antwika::input::KeyPressed;
using antwika::input::MouseButton;
using antwika::input::PointerButtonPressed;
using antwika::input::PointerButtonReleased;
using antwika::input::PointerMoved;
using antwika::input::PointerScrolled;
using antwika::ui::Pointer;
using antwika::ui::WidgetId;

namespace
{
    // The sheet is exactly the canvas.
    // A screen position and an image pixel are then the same number.
    // The toolbar also sits over the art rather than beside it.
    // That is what makes the press-on-the-bar case below a real one.
    constexpr Size kCanvas{.width = 800, .height = 480};

    constexpr Color kClear{
        .red = 0, .green = 0, .blue = 0, .alpha = 0};

    class MemoryStore final : public IAtlasStore
    {
    public:
        std::optional<Bitmap> available{};
        std::optional<Bitmap> written{};
        bool refuseSave = false;
        bool refuseLoad = false;

        std::optional<Bitmap> load() override
        {
            if (refuseLoad)
            {
                throw antwika::gfx::GfxError("no such file");
            }

            return available;
        }

        void save(const Bitmap &image) override
        {
            if (refuseSave)
            {
                throw antwika::atlas_editor::AtlasEditorError(
                    "nowhere to save to");
            }

            written = image;
        }

        [[nodiscard]] std::string savePath() const override
        {
            return "memory.png";
        }
    };

    // The locale is a constant of the build, so a test may name one.
    // What every case here asserts is the English bar's own layout.
    constexpr antwika::i18n::Translator kTranslator{
        antwika::i18n::kDefaultLocale};

    struct Session
    {
        EditorState state{
            Canvas::blank(kCanvas), TileGrid{}, kCanvas};
        UiOverlay overlay{};
        MemoryStore store{};
        InputEventCodec codec{};
    };

    TickEvent tickAt(const std::uint64_t tick)
    {
        return TickEvent{
            .tick = tick,
            .event = Event{.name = antwika::engine::events::kTick}};
    }

    TickEvent inputAt(
        const std::uint64_t tick,
        const InputEvent &event,
        const InputEventCodec &codec)
    {
        return TickEvent{.tick = tick, .event = codec.encode(event)};
    }

    Point middleOf(const Rect &rect)
    {
        return Point{
            .x = rect.origin.x
                 + static_cast<std::int32_t>(rect.size.width / 2),
            .y = rect.origin.y
                 + static_cast<std::int32_t>(rect.size.height / 2)};
    }

    Point widgetPoint(const EditorState &state, const WidgetId widget)
    {
        const auto rect =
            describeEditor(state, Pointer{}, kTranslator)
                .rects.find(widget);

        return middleOf(rect.value_or(Rect{}));
    }

    // Press a toolbar widget.
    // That takes the movement onto it and then the press itself.
    // Those are exactly the events a window system reports.
    void press(
        Session &session,
        EditorSink &sink,
        const WidgetId widget,
        const std::uint64_t tick = 1)
    {
        const Point at = widgetPoint(session.state, widget);

        sink.handle(inputAt(
            tick,
            PointerMoved{.position = {.x = at.x, .y = at.y}},
            session.codec));
        sink.handle(inputAt(
            tick,
            PointerButtonPressed{
                .button = MouseButton::Left,
                .position = {.x = at.x, .y = at.y}},
            session.codec));
        sink.handle(inputAt(
            tick,
            PointerButtonReleased{
                .button = MouseButton::Left,
                .position = {.x = at.x, .y = at.y}},
            session.codec));
    }

    Bitmap oneRedPixel()
    {
        return Bitmap{
            .size = {.width = 1, .height = 1},
            .pixels = {255, 0, 0, 255}};
    }
} // namespace

TEST(EditorSinkTest, Handle_PaintsWhereALeftPressLanded)
{
    Session session;
    EditorSink sink(
        session.state,
        session.overlay,
        session.store,
        session.codec,
        kTranslator);

    sink.handle(inputAt(
        1,
        PointerButtonPressed{
            .button = MouseButton::Left, .position = {.x = 300, .y = 400}},
        session.codec));

    EXPECT_EQ(
        session.state.image().at(Pixel{.x = 300, .y = 400}),
        defaultPalette().front());
    EXPECT_EQ(session.state.edits(), 1U);
}

TEST(EditorSinkTest, Handle_PaintsEveryPixelADragCrosses)
{
    Session session;
    EditorSink sink(
        session.state,
        session.overlay,
        session.store,
        session.codec,
        kTranslator);

    sink.handle(inputAt(
        1,
        PointerButtonPressed{
            .button = MouseButton::Left, .position = {.x = 100, .y = 300}},
        session.codec));
    sink.handle(inputAt(
        1,
        PointerMoved{.position = {.x = 101, .y = 300}},
        session.codec));
    sink.handle(inputAt(
        2,
        PointerMoved{.position = {.x = 102, .y = 300}},
        session.codec));
    sink.handle(inputAt(
        2,
        PointerButtonReleased{
            .button = MouseButton::Left, .position = {.x = 102, .y = 300}},
        session.codec));

    EXPECT_EQ(session.state.edits(), 3U);

    // And nothing after the button came up.
    sink.handle(inputAt(
        3,
        PointerMoved{.position = {.x = 103, .y = 300}},
        session.codec));
    EXPECT_EQ(session.state.edits(), 3U);
}

TEST(EditorSinkTest, Handle_ErasesUnderTheRightButton)
{
    Session session;
    EditorSink sink(
        session.state,
        session.overlay,
        session.store,
        session.codec,
        kTranslator);

    sink.handle(inputAt(
        1,
        PointerButtonPressed{
            .button = MouseButton::Left, .position = {.x = 50, .y = 200}},
        session.codec));
    sink.handle(inputAt(
        2,
        PointerButtonReleased{
            .button = MouseButton::Left, .position = {.x = 50, .y = 200}},
        session.codec));
    sink.handle(inputAt(
        3,
        PointerButtonPressed{
            .button = MouseButton::Right, .position = {.x = 50, .y = 200}},
        session.codec));

    EXPECT_EQ(session.state.image().at(Pixel{.x = 50, .y = 200}), kClear);
    EXPECT_EQ(session.state.edits(), 2U);
}

TEST(EditorSinkTest, Handle_PansUnderTheMiddleButtonAndPaintsNothing)
{
    Session session;
    EditorSink sink(
        session.state,
        session.overlay,
        session.store,
        session.codec,
        kTranslator);

    const Point before = session.state.view().pan;

    sink.handle(inputAt(
        1,
        PointerButtonPressed{
            .button = MouseButton::Middle,
            .position = {.x = 200, .y = 300}},
        session.codec));
    sink.handle(inputAt(
        1,
        PointerMoved{.position = {.x = 220, .y = 290}},
        session.codec));

    EXPECT_EQ(
        session.state.view().pan,
        (Point{.x = before.x + 20, .y = before.y - 10}));
    EXPECT_EQ(session.state.edits(), 0U);
}

TEST(EditorSinkTest, Handle_ZoomsOnTheWheel)
{
    Session session;
    EditorSink sink(
        session.state,
        session.overlay,
        session.store,
        session.codec,
        kTranslator);

    sink.handle(inputAt(
        1,
        PointerMoved{.position = {.x = 400, .y = 300}},
        session.codec));
    sink.handle(inputAt(
        1, PointerScrolled{.horizontal = 0, .vertical = 1},
        session.codec));

    EXPECT_EQ(scaleOf(session.state.view()), 2U);

    sink.handle(inputAt(
        2, PointerScrolled{.horizontal = 0, .vertical = -1},
        session.codec));

    EXPECT_EQ(scaleOf(session.state.view()), 1U);

    // A horizontal notch alone is neither, and must not zoom.
    sink.handle(inputAt(
        3, PointerScrolled{.horizontal = 2, .vertical = 0},
        session.codec));

    EXPECT_EQ(scaleOf(session.state.view()), 1U);
}

TEST(EditorSinkTest, Handle_LeavesTheArtAloneUnderTheToolbar)
{
    Session session;
    EditorSink sink(
        session.state,
        session.overlay,
        session.store,
        session.codec,
        kTranslator);

    namespace widgets = antwika::atlas_editor::widgets;
    press(session, sink, widgets::kGrid);

    EXPECT_FALSE(session.state.gridVisible());
    EXPECT_EQ(session.state.edits(), 0U);
}

TEST(EditorSinkTest, Handle_SelectsAToolAndAColourFromTheBar)
{
    Session session;
    EditorSink sink(
        session.state,
        session.overlay,
        session.store,
        session.codec,
        kTranslator);

    namespace widgets = antwika::atlas_editor::widgets;
    press(session, sink, widgets::toolWidget(Tool::Pick));
    EXPECT_EQ(session.state.tool(), Tool::Pick);

    press(session, sink, widgets::swatchWidget(6), 2);
    EXPECT_EQ(session.state.colorIndex(), 6U);
}

TEST(EditorSinkTest, Handle_ZoomsAndRecentresFromTheBar)
{
    Session session;
    EditorSink sink(
        session.state,
        session.overlay,
        session.store,
        session.codec,
        kTranslator);

    namespace widgets = antwika::atlas_editor::widgets;
    press(session, sink, widgets::kZoomIn);
    EXPECT_EQ(scaleOf(session.state.view()), 2U);

    press(session, sink, widgets::kZoomOut, 2);
    EXPECT_EQ(scaleOf(session.state.view()), 1U);

    session.state.panBy(Point{.x = 40, .y = 40});
    press(session, sink, widgets::kResetView, 3);
    EXPECT_EQ(session.state.view().pan, (Point{}));
}

TEST(EditorSinkTest, Handle_SavesTheSheetThroughTheStore)
{
    Session session;
    EditorSink sink(
        session.state,
        session.overlay,
        session.store,
        session.codec,
        kTranslator);

    sink.handle(inputAt(
        1,
        PointerButtonPressed{
            .button = MouseButton::Left, .position = {.x = 10, .y = 400}},
        session.codec));
    sink.handle(inputAt(
        1,
        PointerButtonReleased{
            .button = MouseButton::Left, .position = {.x = 10, .y = 400}},
        session.codec));

    namespace widgets = antwika::atlas_editor::widgets;
    press(session, sink, widgets::kSave, 2);

    ASSERT_TRUE(session.store.written.has_value());
    EXPECT_EQ(*session.store.written, session.state.image().bitmap());
    EXPECT_EQ(session.state.saves(), 1U);
    EXPECT_FALSE(session.state.unsaved());
    // The id and the path, rather than the sentence they make.
    // What a status *is* is what the state now holds.
    // The words are EditorUi's, and are asserted there.
    ASSERT_TRUE(session.state.status().has_value());
    EXPECT_EQ(session.state.status()->id, MessageId::AtlasSaved);
    EXPECT_EQ(session.state.status()->detail, "memory.png");
}

TEST(EditorSinkTest, Handle_KeepsTheSessionAliveWhenASaveFails)
{
    Session session;
    session.store.refuseSave = true;
    EditorSink sink(
        session.state,
        session.overlay,
        session.store,
        session.codec,
        kTranslator);

    namespace widgets = antwika::atlas_editor::widgets;
    press(session, sink, widgets::kSave);

    EXPECT_EQ(session.state.saves(), 0U);
    ASSERT_TRUE(session.state.status().has_value());
    EXPECT_EQ(session.state.status()->id, MessageId::AtlasSaveFailed);
    EXPECT_FALSE(session.state.status()->detail.empty());
}

TEST(EditorSinkTest, Handle_LoadsASheetThroughTheStore)
{
    Session session;
    session.store.available = oneRedPixel();
    EditorSink sink(
        session.state,
        session.overlay,
        session.store,
        session.codec,
        kTranslator);

    namespace widgets = antwika::atlas_editor::widgets;
    press(session, sink, widgets::kLoad);

    EXPECT_EQ(
        session.state.image().size(), (Size{.width = 1, .height = 1}));
    EXPECT_EQ(session.state.loads(), 1U);
    ASSERT_TRUE(session.state.status().has_value());
    EXPECT_EQ(session.state.status()->id, MessageId::AtlasLoaded);
}

TEST(EditorSinkTest, Handle_SaysSoWhenThereIsNothingToLoad)
{
    Session session;
    EditorSink sink(
        session.state,
        session.overlay,
        session.store,
        session.codec,
        kTranslator);

    namespace widgets = antwika::atlas_editor::widgets;
    press(session, sink, widgets::kLoad);

    EXPECT_EQ(session.state.loads(), 0U);
    ASSERT_TRUE(session.state.status().has_value());
    EXPECT_EQ(
        session.state.status()->id, MessageId::AtlasNothingToLoad);
}

TEST(EditorSinkTest, Handle_KeepsTheSessionAliveWhenALoadFails)
{
    Session session;
    session.store.refuseLoad = true;
    EditorSink sink(
        session.state,
        session.overlay,
        session.store,
        session.codec,
        kTranslator);

    namespace widgets = antwika::atlas_editor::widgets;
    press(session, sink, widgets::kLoad);

    EXPECT_EQ(session.state.loads(), 0U);
    ASSERT_TRUE(session.state.status().has_value());
    EXPECT_EQ(session.state.status()->id, MessageId::AtlasLoadFailed);
    EXPECT_FALSE(session.state.status()->detail.empty());
}

// A key edge is the one input saying nothing about the pointer.
// Nothing on the sheet may move on it.
TEST(EditorSinkTest, Handle_PaintsNothingOnAKeyThatSaysNoPosition)
{
    Session session;
    EditorSink sink(
        session.state,
        session.overlay,
        session.store,
        session.codec,
        kTranslator);

    sink.handle(inputAt(
        1,
        PointerButtonPressed{
            .button = MouseButton::Left, .position = {.x = 20, .y = 300}},
        session.codec));

    const auto before = session.state.edits();

    sink.handle(inputAt(
        1, KeyPressed{.key = Key::A}, session.codec));

    EXPECT_EQ(session.state.edits(), before);
}

TEST(EditorSinkTest, Handle_DescribesTheBarForTheRendererOnEveryTick)
{
    Session session;
    EditorSink sink(
        session.state,
        session.overlay,
        session.store,
        session.codec,
        kTranslator);

    EXPECT_TRUE(session.overlay.commands().empty());

    sink.handle(tickAt(1));

    EXPECT_FALSE(session.overlay.commands().empty());
    EXPECT_EQ(session.state.ticks(), 1U);
}

TEST(EditorSinkTest, Handle_IgnoresAnEventThatIsNeitherATickNorInput)
{
    Session session;
    EditorSink sink(
        session.state,
        session.overlay,
        session.store,
        session.codec,
        kTranslator);

    sink.handle(TickEvent{
        .tick = 1, .event = Event{.name = "atlas_editor.nothing"}});

    EXPECT_EQ(session.state.ticks(), 0U);
    EXPECT_EQ(session.state.edits(), 0U);
    EXPECT_TRUE(session.overlay.commands().empty());
}
