#include <gtest/gtest.h>

#include <cstdint>
#include <filesystem>
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
#include <antwika/input/InputEvent.hpp>
#include <antwika/input/InputEventCodec.hpp>
#include <antwika/input/Key.hpp>
#include <antwika/input/MouseButton.hpp>
#include <antwika/testing/ScratchPath.hpp>
#include <antwika/ui/Pointer.hpp>

#include "antwika/atlas_editor/AtlasEditorError.hpp"
#include "antwika/atlas_editor/AtlasForm.hpp"
#include "antwika/atlas_editor/Canvas.hpp"
#include "antwika/atlas_editor/CanvasView.hpp"
#include "antwika/atlas_editor/EditorSink.hpp"
#include "antwika/atlas_editor/EditorState.hpp"
#include "antwika/atlas_editor/EditorTheme.hpp"
#include "antwika/atlas_editor/EditorUi.hpp"
#include "antwika/atlas_editor/FileList.hpp"
#include "antwika/atlas_editor/AtlasMeta.hpp"
#include "antwika/atlas_editor/IAtlasStore.hpp"
#include "antwika/atlas_editor/MessageId.hpp"
#include "antwika/atlas_editor/Messages.hpp"
#include "antwika/atlas_editor/Palette.hpp"
#include "antwika/atlas_editor/Pixel.hpp"
#include "antwika/atlas_editor/TileGrid.hpp"
#include "antwika/atlas_editor/Tool.hpp"
#include "antwika/atlas_editor/UiOverlay.hpp"

using antwika::atlas_editor::Canvas;
using antwika::atlas_editor::defaultPalette;
using antwika::atlas_editor::describeEditor;
using antwika::atlas_editor::EditorSink;
using antwika::atlas_editor::EditorState;
using antwika::atlas_editor::FileEntry;
using antwika::atlas_editor::IAtlasStore;
using antwika::atlas_editor::MessageId;
using antwika::atlas_editor::Pixel;
using antwika::atlas_editor::scaleOf;
using antwika::atlas_editor::TileGrid;
using antwika::atlas_editor::Tool;
using antwika::atlas_editor::UiOverlay;
using antwika::testing::ScratchDirectory;
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
    constexpr Size kCanvas{.width = 800, .height = 480};

    const std::size_t kFileRows = antwika::atlas_editor::filesShownIn(
        kCanvas, antwika::atlas_editor::kCardLabels);

    constexpr Color kClear{
        .red = 0, .green = 0, .blue = 0, .alpha = 0};

    class FakeMemoryStore final : public IAtlasStore
    {
    public:
        std::optional<Bitmap> available{};
        std::optional<Bitmap> written{};
        bool refuseSave = false;
        bool refuseLoad = false;
        std::optional<std::string> opened{};
        std::optional<std::string> wrote{};

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
    };

    constexpr antwika::atlas_editor::Translator kTranslator{
        antwika::i18n::kDefaultLocale};

    class FakeStopWatcher final : public antwika::event::ITickEventSink
    {
    public:
        void handle(const TickEvent &event) override
        {
            names.push_back(event.event.name);
        }

        std::vector<std::string> names{};
    };

    struct Session final
    {
        EditorState state{
            Canvas::blank(kCanvas), TileGrid{}, kCanvas};
        UiOverlay overlay{};
        FakeMemoryStore store{};
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

    void dragOnSheet(
        Session &session,
        EditorSink &sink,
        const Point from,
        const Point to,
        const MouseButton button = MouseButton::Left,
        const std::uint64_t tick = 2)
    {
        sink.handle(inputAt(
            tick,
            PointerButtonPressed{
                .button = button,
                .position = {.x = from.x, .y = from.y}},
            session.codec));
        sink.handle(inputAt(
            tick,
            PointerMoved{.position = {.x = to.x, .y = to.y}},
            session.codec));
        sink.handle(inputAt(
            tick,
            PointerButtonReleased{
                .button = button, .position = {.x = to.x, .y = to.y}},
            session.codec));
    }

    void chooseFile(
        Session &session,
        EditorSink &sink,
        const antwika::atlas_editor::FileItem item,
        const std::uint64_t tick = 1)
    {
        namespace widgets = antwika::atlas_editor::widgets;

        press(session, sink, widgets::kFileMenu, tick);
        press(session, sink, widgets::fileItemWidget(item), tick);
    }

    void chooseView(
        Session &session,
        EditorSink &sink,
        const antwika::atlas_editor::ViewItem item,
        const std::uint64_t tick = 1)
    {
        namespace widgets = antwika::atlas_editor::widgets;

        press(session, sink, widgets::kViewMenu, tick);
        press(session, sink, widgets::viewItemWidget(item), tick);
    }

    void confirmFile(
        Session &session,
        EditorSink &sink,
        const std::string &name,
        const std::uint64_t tick)
    {
        namespace widgets = antwika::atlas_editor::widgets;

        session.state.setFileName(name, 0);
        press(session, sink, widgets::kFileConfirm, tick);
    }

    void saveThrough(
        Session &session,
        EditorSink &sink,
        const std::string &name = "sheet.png",
        const std::uint64_t tick = 1)
    {
        chooseFile(
            session, sink, antwika::atlas_editor::FileItem::Save, tick);
        confirmFile(session, sink, name, tick);
    }

    void loadThrough(
        Session &session,
        EditorSink &sink,
        const std::string &name = "sheet.png",
        const std::uint64_t tick = 1)
    {
        chooseFile(
            session, sink, antwika::atlas_editor::FileItem::Load, tick);
        confirmFile(session, sink, name, tick);
    }

    void fillField(
        Session &session,
        EditorSink &sink,
        const antwika::atlas_editor::AtlasField field,
        const std::string &text,
        const std::uint64_t tick = 1)
    {
        namespace widgets = antwika::atlas_editor::widgets;

        press(
            session,
            sink,
            widgets::atlasFieldWidget(
                static_cast<std::size_t>(field)),
            tick);
        session.state.setFormField(text, 0);
    }

    void chord(
        Session &session,
        EditorSink &sink,
        const Key key,
        const std::uint64_t tick = 3,
        const bool shift = false)
    {
        sink.handle(inputAt(
            tick,
            KeyPressed{
                .key = key,
                .modifiers = {.shift = shift, .control = true}},
            session.codec));
    }

    void markOut(
        Session &session,
        EditorSink &sink,
        const Point from,
        const Point to)
    {
        namespace widgets = antwika::atlas_editor::widgets;
        press(session, sink, widgets::toolWidget(Tool::Select));
        dragOnSheet(session, sink, from, to);
    }

    Bitmap oneRedPixel()
    {
        return Bitmap{
            .size = {.width = 1, .height = 1},
            .pixels = {255, 0, 0, 255}};
    }

    Bitmap blankSheet(const Size size)
    {
        return Bitmap{
            .size = size,
            .pixels = std::vector<std::uint8_t>(
                static_cast<std::size_t>(size.width) * size.height * 4,
                0)};
    }
}

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

    sink.handle(inputAt(
        3,
        PointerMoved{.position = {.x = 103, .y = 300}},
        session.codec));
    EXPECT_EQ(session.state.edits(), 3U);
}

TEST(EditorSinkTest, Handle_PaintsTheWholeSegmentOfAJumpedDrag)
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
        PointerMoved{.position = {.x = 106, .y = 300}},
        session.codec));

    for (std::int32_t x = 100; x <= 106; ++x)
    {
        EXPECT_EQ(
            session.state.image().at(Pixel{.x = x, .y = 300}),
            defaultPalette().front())
            << x;
    }

    EXPECT_EQ(session.state.edits(), 7U);
}

TEST(EditorSinkTest, Handle_PaintsADiagonalJumpAsAConnectedLine)
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
            .button = MouseButton::Left, .position = {.x = 210, .y = 210}},
        session.codec));
    sink.handle(inputAt(
        1,
        PointerMoved{.position = {.x = 206, .y = 206}},
        session.codec));

    for (std::int32_t step = 0; step <= 4; ++step)
    {
        EXPECT_EQ(
            session.state.image().at(
                Pixel{.x = 210 - step, .y = 210 - step}),
            defaultPalette().front());
    }

    EXPECT_EQ(session.state.edits(), 5U);
}

TEST(EditorSinkTest, Handle_ErasesTheWholeSegmentOfAJumpedDrag)
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
            .button = MouseButton::Left, .position = {.x = 400, .y = 200}},
        session.codec));
    sink.handle(inputAt(
        1,
        PointerMoved{.position = {.x = 400, .y = 205}},
        session.codec));
    sink.handle(inputAt(
        2,
        PointerButtonReleased{
            .button = MouseButton::Left, .position = {.x = 400, .y = 205}},
        session.codec));

    ASSERT_EQ(session.state.edits(), 6U);

    sink.handle(inputAt(
        3,
        PointerButtonPressed{
            .button = MouseButton::Right,
            .position = {.x = 400, .y = 205}},
        session.codec));
    sink.handle(inputAt(
        3,
        PointerMoved{.position = {.x = 400, .y = 200}},
        session.codec));

    for (std::int32_t y = 200; y <= 205; ++y)
    {
        EXPECT_EQ(
            session.state.image().at(Pixel{.x = 400, .y = y}), kClear)
            << y;
    }

    EXPECT_EQ(session.state.edits(), 12U);
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
            .button = MouseButton::Left,
            .position = {.x = 150, .y = 200}},
        session.codec));
    sink.handle(inputAt(
        2,
        PointerButtonReleased{
            .button = MouseButton::Left,
            .position = {.x = 150, .y = 200}},
        session.codec));
    sink.handle(inputAt(
        3,
        PointerButtonPressed{
            .button = MouseButton::Right,
            .position = {.x = 150, .y = 200}},
        session.codec));

    EXPECT_EQ(session.state.image().at(Pixel{.x = 150, .y = 200}), kClear);
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

    chooseView(session, sink, antwika::atlas_editor::ViewItem::Grid);

    EXPECT_FALSE(session.state.gridVisible());
    EXPECT_EQ(session.state.edits(), 0U);
}

TEST(EditorSinkTest, Handle_TurnsTheSpriteGuidesOffFromTheBar)
{
    Session session;
    EditorSink sink(
        session.state,
        session.overlay,
        session.store,
        session.codec,
        kTranslator);

    chooseView(session, sink, antwika::atlas_editor::ViewItem::Guides);

    EXPECT_FALSE(session.state.guidesVisible());
    EXPECT_TRUE(session.state.gridVisible());
    EXPECT_EQ(session.state.edits(), 0U);
}

TEST(EditorSinkTest, Handle_TurnsThePixelGridOnFromTheViewMenu)
{
    Session session;
    EditorSink sink(
        session.state,
        session.overlay,
        session.store,
        session.codec,
        kTranslator);

    ASSERT_FALSE(session.state.pixelGridVisible());

    chooseView(
        session, sink, antwika::atlas_editor::ViewItem::PixelGrid);

    EXPECT_TRUE(session.state.pixelGridVisible());
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

    using antwika::atlas_editor::ViewItem;

    chooseView(session, sink, ViewItem::ZoomIn);
    EXPECT_EQ(scaleOf(session.state.view()), 2U);

    chooseView(session, sink, ViewItem::ZoomOut, 2);
    EXPECT_EQ(scaleOf(session.state.view()), 1U);

    session.state.panBy(Point{.x = 40, .y = 40});
    chooseView(session, sink, ViewItem::Fit, 3);
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

    saveThrough(session, sink, "sheet.png", 2);

    ASSERT_TRUE(session.store.written.has_value());
    EXPECT_EQ(*session.store.written, session.state.image().bitmap());
    EXPECT_EQ(session.state.saves(), 1U);
    EXPECT_FALSE(session.state.unsaved());
    ASSERT_TRUE(session.state.status().has_value());
    EXPECT_EQ(session.state.status()->id, MessageId::Saved);
    EXPECT_EQ(session.state.status()->detail, "sheet.png");
}

TEST(EditorSinkTest, Handle_OpensTheFileModalFromTheSaveMenuItem)
{
    Session session;
    EditorSink sink(
        session.state,
        session.overlay,
        session.store,
        session.codec,
        kTranslator);

    chooseFile(
        session, sink, antwika::atlas_editor::FileItem::Save);

    EXPECT_EQ(
        session.state.openModal(), antwika::atlas_editor::Modal::Save);
    EXPECT_FALSE(session.store.written.has_value());
}

TEST(EditorSinkTest, Handle_ShutsTheFileModalOnTheCloseButton)
{
    namespace widgets = antwika::atlas_editor::widgets;

    Session session;
    EditorSink sink(
        session.state,
        session.overlay,
        session.store,
        session.codec,
        kTranslator);

    chooseFile(
        session, sink, antwika::atlas_editor::FileItem::Load);
    press(session, sink, widgets::kFileClose, 2);

    EXPECT_EQ(
        session.state.openModal(), antwika::atlas_editor::Modal::None);
}

TEST(EditorSinkTest, Handle_TakesTheFileNameFromTheEntryItWasGiven)
{
    namespace widgets = antwika::atlas_editor::widgets;

    Session session;
    EditorSink sink(
        session.state,
        session.overlay,
        session.store,
        session.codec,
        kTranslator);

    session.state.showModal(
        antwika::atlas_editor::Modal::Load,
        ".",
        {FileEntry{.name = "one.png"}, FileEntry{.name = "two.png"}});

    press(session, sink, widgets::fileEntryWidget(1), 2);

    EXPECT_EQ(session.state.fileName(), "two.png");
    EXPECT_EQ(
        session.state.openModal(), antwika::atlas_editor::Modal::Load);
}

TEST(EditorSinkTest, Handle_TypesTheFileNameIntoTheModalsField)
{
    Session session;
    EditorSink sink(
        session.state,
        session.overlay,
        session.store,
        session.codec,
        kTranslator);

    session.state.showModal(
        antwika::atlas_editor::Modal::Save, ".", {});

    sink.handle(inputAt(
        2, KeyPressed{.key = Key::A}, session.codec));
    sink.handle(inputAt(
        3, KeyPressed{.key = Key::B}, session.codec));

    EXPECT_EQ(session.state.fileName(), "ab");
}

TEST(EditorSinkTest, Handle_SavesOnTheEnterKeyInTheModalsField)
{
    Session session;
    EditorSink sink(
        session.state,
        session.overlay,
        session.store,
        session.codec,
        kTranslator);

    session.state.showModal(
        antwika::atlas_editor::Modal::Save, ".", {});
    session.state.setFileName("typed.png", 0);

    sink.handle(inputAt(
        2, KeyPressed{.key = Key::Enter}, session.codec));

    ASSERT_TRUE(session.store.wrote.has_value());
    EXPECT_EQ(*session.store.wrote, "typed.png");
}

TEST(EditorSinkTest, Handle_RubsOutTheLastLetterOfTheFileName)
{
    Session session;
    EditorSink sink(
        session.state,
        session.overlay,
        session.store,
        session.codec,
        kTranslator);

    session.state.showModal(
        antwika::atlas_editor::Modal::Save, ".", {});
    session.state.setFileName(
        "ab", antwika::ui::kCaretAtEnd);

    sink.handle(inputAt(
        2, KeyPressed{.key = Key::Backspace}, session.codec));

    EXPECT_EQ(session.state.fileName(), "a");
}

TEST(EditorSinkTest, Handle_TypesNothingForAKeyTheBoardHasNoLetterFor)
{
    Session session;
    EditorSink sink(
        session.state,
        session.overlay,
        session.store,
        session.codec,
        kTranslator);

    session.state.showModal(
        antwika::atlas_editor::Modal::Save, ".", {});

    sink.handle(inputAt(
        2, KeyPressed{.key = Key::F1}, session.codec));

    EXPECT_TRUE(session.state.fileName().empty());
}

TEST(EditorSinkTest, Handle_TakesNoFileFromAPressBesideTheEntries)
{
    namespace widgets = antwika::atlas_editor::widgets;

    Session session;
    EditorSink sink(
        session.state,
        session.overlay,
        session.store,
        session.codec,
        kTranslator);

    session.state.showModal(
        antwika::atlas_editor::Modal::Load,
        ".",
        {FileEntry{.name = "one.png"}});

    press(session, sink, widgets::kFileField, 2);

    EXPECT_TRUE(session.state.fileName().empty());
    EXPECT_EQ(
        session.state.openModal(), antwika::atlas_editor::Modal::Load);
}

TEST(EditorSinkTest, Handle_SaysSoWhenTheNamedFileHoldsNoImage)
{
    Session session;
    EditorSink sink(
        session.state,
        session.overlay,
        session.store,
        session.codec,
        kTranslator);

    loadThrough(session, sink, "empty.png");

    ASSERT_TRUE(session.state.status().has_value());
    EXPECT_EQ(
        session.state.status()->id, MessageId::NothingToLoad);
}

TEST(EditorSinkTest, Handle_SavesToNoFileWithNoNameGiven)
{
    namespace widgets = antwika::atlas_editor::widgets;

    Session session;
    EditorSink sink(
        session.state,
        session.overlay,
        session.store,
        session.codec,
        kTranslator);

    chooseFile(
        session, sink, antwika::atlas_editor::FileItem::Save);
    press(session, sink, widgets::kFileConfirm, 2);

    EXPECT_FALSE(session.store.written.has_value());
    EXPECT_EQ(
        session.state.openModal(), antwika::atlas_editor::Modal::Save);
}

TEST(EditorSinkTest, Handle_SavesToTheFileTheModalNamed)
{
    Session session;
    EditorSink sink(
        session.state,
        session.overlay,
        session.store,
        session.codec,
        kTranslator);

    saveThrough(session, sink, "named.png");

    ASSERT_TRUE(session.store.wrote.has_value());
    EXPECT_EQ(*session.store.wrote, "named.png");
    EXPECT_EQ(
        session.state.openModal(), antwika::atlas_editor::Modal::None);
}

TEST(EditorSinkTest, Handle_LoadsFromTheFileTheModalNamed)
{
    Session session;
    session.store.available =
        Canvas::blank(Size{.width = 4, .height = 4}).bitmap();

    EditorSink sink(
        session.state,
        session.overlay,
        session.store,
        session.codec,
        kTranslator);

    loadThrough(session, sink, "named.png");

    ASSERT_TRUE(session.store.opened.has_value());
    EXPECT_EQ(*session.store.opened, "named.png");
    EXPECT_EQ(session.state.image().size(), (Size{.width = 4, .height = 4}));
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

    saveThrough(session, sink);

    EXPECT_EQ(session.state.saves(), 0U);
    ASSERT_TRUE(session.state.status().has_value());
    EXPECT_EQ(session.state.status()->id, MessageId::SaveFailed);
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

    loadThrough(session, sink);

    EXPECT_EQ(
        session.state.image().size(), (Size{.width = 1, .height = 1}));
    EXPECT_EQ(session.state.loads(), 1U);
    ASSERT_TRUE(session.state.status().has_value());
    EXPECT_EQ(session.state.status()->id, MessageId::Loaded);
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

    loadThrough(session, sink);

    EXPECT_EQ(session.state.loads(), 0U);
    ASSERT_TRUE(session.state.status().has_value());
    EXPECT_EQ(
        session.state.status()->id, MessageId::NothingToLoad);
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

    loadThrough(session, sink);

    EXPECT_EQ(session.state.loads(), 0U);
    ASSERT_TRUE(session.state.status().has_value());
    EXPECT_EQ(session.state.status()->id, MessageId::LoadFailed);
    EXPECT_FALSE(session.state.status()->detail.empty());
}

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

TEST(EditorSinkTest, Handle_MarksOutTheRectangleADragCrossed)
{
    Session session;
    EditorSink sink(
        session.state,
        session.overlay,
        session.store,
        session.codec,
        kTranslator);

    markOut(
        session,
        sink,
        Point{.x = 200, .y = 300},
        Point{.x = 203, .y = 302});

    ASSERT_TRUE(session.state.selection().has_value());
    EXPECT_EQ(
        session.state.selection()->origin, (Pixel{.x = 200, .y = 300}));
    EXPECT_EQ(
        session.state.selection()->size, (Size{.width = 4, .height = 3}));

    EXPECT_EQ(session.state.edits(), 0U);
}

TEST(EditorSinkTest, Handle_DrawsTheLineADragCrossedWhenItIsLetGo)
{
    namespace widgets = antwika::atlas_editor::widgets;

    Session session;
    EditorSink sink(
        session.state,
        session.overlay,
        session.store,
        session.codec,
        kTranslator);

    press(session, sink, widgets::toolWidget(Tool::Line));
    dragOnSheet(
        session,
        sink,
        Point{.x = 200, .y = 300},
        Point{.x = 204, .y = 300});

    EXPECT_EQ(
        session.state.image().at(Pixel{.x = 202, .y = 300}),
        defaultPalette().front());
    EXPECT_EQ(session.state.edits(), 5U);
}

TEST(EditorSinkTest, Handle_LeavesTheSheetAloneUntilTheLineIsLetGo)
{
    namespace widgets = antwika::atlas_editor::widgets;

    Session session;
    EditorSink sink(
        session.state,
        session.overlay,
        session.store,
        session.codec,
        kTranslator);

    press(session, sink, widgets::toolWidget(Tool::Line));

    sink.handle(inputAt(
        2,
        PointerButtonPressed{
            .button = MouseButton::Left,
            .position = {.x = 200, .y = 300}},
        session.codec));
    sink.handle(inputAt(
        2,
        PointerMoved{.position = {.x = 204, .y = 300}},
        session.codec));

    EXPECT_EQ(session.state.edits(), 0U);
    ASSERT_TRUE(session.state.shownStroke().has_value());
    EXPECT_EQ(
        session.state.shownStroke()->to, (Pixel{.x = 204, .y = 300}));
}

TEST(EditorSinkTest, Handle_TracksAShapeToolsPointerWithNoButtonDown)
{
    namespace widgets = antwika::atlas_editor::widgets;

    Session session;
    EditorSink sink(
        session.state,
        session.overlay,
        session.store,
        session.codec,
        kTranslator);

    press(session, sink, widgets::toolWidget(Tool::Line));

    sink.handle(inputAt(
        2,
        PointerMoved{.position = {.x = 204, .y = 300}},
        session.codec));

    EXPECT_EQ(session.state.edits(), 0U);
    EXPECT_FALSE(session.state.shownStroke().has_value());
    EXPECT_EQ(session.state.hovered(), (Pixel{.x = 204, .y = 300}));
}

TEST(EditorSinkTest, Handle_StillErasesOnTheRightWithAShapeToolInHand)
{
    namespace widgets = antwika::atlas_editor::widgets;

    Session session;
    EditorSink sink(
        session.state,
        session.overlay,
        session.store,
        session.codec,
        kTranslator);

    press(session, sink, widgets::toolWidget(Tool::Ellipse));
    dragOnSheet(
        session,
        sink,
        Point{.x = 200, .y = 300},
        Point{.x = 206, .y = 306});

    const auto drawn = session.state.edits();
    ASSERT_GT(drawn, 0U);

    sink.handle(inputAt(
        3,
        PointerButtonPressed{
            .button = MouseButton::Right,
            .position = {.x = 203, .y = 300}},
        session.codec));

    EXPECT_EQ(session.state.image().at(Pixel{.x = 203, .y = 300}), kClear);
    EXPECT_GT(session.state.edits(), drawn);
}

TEST(EditorSinkTest, Handle_CarriesAMarkedRectangleToWhereADragEnded)
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
            .button = MouseButton::Left, .position = {.x = 200, .y = 300}},
        session.codec));

    markOut(
        session,
        sink,
        Point{.x = 200, .y = 300},
        Point{.x = 200, .y = 300});

    dragOnSheet(
        session,
        sink,
        Point{.x = 200, .y = 300},
        Point{.x = 260, .y = 340},
        MouseButton::Left,
        4);

    EXPECT_EQ(session.state.image().at(Pixel{.x = 200, .y = 300}), kClear);
    EXPECT_EQ(
        session.state.image().at(Pixel{.x = 260, .y = 340}),
        defaultPalette().front());
}

TEST(EditorSinkTest, Handle_ClearsTheSelectionOnARightPress)
{
    Session session;
    EditorSink sink(
        session.state,
        session.overlay,
        session.store,
        session.codec,
        kTranslator);

    markOut(
        session,
        sink,
        Point{.x = 200, .y = 300},
        Point{.x = 210, .y = 310});
    ASSERT_TRUE(session.state.selection().has_value());

    sink.handle(inputAt(
        5,
        PointerButtonPressed{
            .button = MouseButton::Right, .position = {.x = 400, .y = 300}},
        session.codec));

    EXPECT_FALSE(session.state.selection().has_value());

    EXPECT_EQ(session.state.edits(), 0U);
}

TEST(EditorSinkTest, Handle_StillErasesOnTheRightWithABrushInHand)
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
            .button = MouseButton::Left, .position = {.x = 200, .y = 300}},
        session.codec));
    sink.handle(inputAt(
        1,
        PointerButtonReleased{
            .button = MouseButton::Left, .position = {.x = 200, .y = 300}},
        session.codec));
    EXPECT_EQ(session.state.edits(), 1U);

    sink.handle(inputAt(
        2,
        PointerButtonPressed{
            .button = MouseButton::Right, .position = {.x = 200, .y = 300}},
        session.codec));

    EXPECT_EQ(session.state.image().at(Pixel{.x = 200, .y = 300}), kClear);
    EXPECT_EQ(session.state.edits(), 2U);
}

TEST(EditorSinkTest, Handle_CopiesAndPastesWithControlCAndControlV)
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
            .button = MouseButton::Left, .position = {.x = 200, .y = 300}},
        session.codec));

    markOut(
        session,
        sink,
        Point{.x = 200, .y = 300},
        Point{.x = 200, .y = 300});

    chord(session, sink, Key::C);

    sink.handle(inputAt(
        4,
        PointerMoved{.position = {.x = 400, .y = 350}},
        session.codec));
    chord(session, sink, Key::V, 5);

    EXPECT_EQ(
        session.state.image().at(Pixel{.x = 200, .y = 300}),
        defaultPalette().front());
    EXPECT_EQ(
        session.state.image().at(Pixel{.x = 400, .y = 350}),
        defaultPalette().front());
}

TEST(EditorSinkTest, Handle_CutsWithControlX)
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
            .button = MouseButton::Left, .position = {.x = 200, .y = 300}},
        session.codec));

    markOut(
        session,
        sink,
        Point{.x = 200, .y = 300},
        Point{.x = 200, .y = 300});

    chord(session, sink, Key::X);

    EXPECT_EQ(session.state.image().at(Pixel{.x = 200, .y = 300}), kClear);

    sink.handle(inputAt(
        4,
        PointerMoved{.position = {.x = 400, .y = 350}},
        session.codec));
    chord(session, sink, Key::V, 5);

    EXPECT_EQ(
        session.state.image().at(Pixel{.x = 400, .y = 350}),
        defaultPalette().front());
}

TEST(EditorSinkTest, Handle_UndoesAStrokeWithControlZ)
{
    Session session;
    EditorSink sink(
        session.state,
        session.overlay,
        session.store,
        session.codec,
        kTranslator);

    dragOnSheet(
        session,
        sink,
        Point{.x = 200, .y = 300},
        Point{.x = 203, .y = 300});

    ASSERT_EQ(session.state.undoDepth(), 1U);
    ASSERT_NE(
        session.state.image().at(Pixel{.x = 202, .y = 300}), kClear);

    chord(session, sink, Key::Z);

    EXPECT_EQ(
        session.state.image().at(Pixel{.x = 202, .y = 300}), kClear);
    EXPECT_EQ(session.state.undoDepth(), 0U);
    EXPECT_EQ(session.state.redoDepth(), 1U);
}

TEST(EditorSinkTest, Handle_RedoesAnUndoneStrokeWithControlShiftZ)
{
    Session session;
    EditorSink sink(
        session.state,
        session.overlay,
        session.store,
        session.codec,
        kTranslator);

    dragOnSheet(
        session,
        sink,
        Point{.x = 200, .y = 300},
        Point{.x = 203, .y = 300});

    const auto painted =
        session.state.image().at(Pixel{.x = 202, .y = 300});

    chord(session, sink, Key::Z);
    chord(session, sink, Key::Z, 4, true);

    EXPECT_EQ(
        session.state.image().at(Pixel{.x = 202, .y = 300}), painted);
    EXPECT_EQ(session.state.redoDepth(), 0U);
}

TEST(EditorSinkTest, Handle_KeepsOneStrokePerDragInTheUndoRecord)
{
    Session session;
    EditorSink sink(
        session.state,
        session.overlay,
        session.store,
        session.codec,
        kTranslator);

    dragOnSheet(
        session,
        sink,
        Point{.x = 200, .y = 300},
        Point{.x = 208, .y = 300});
    dragOnSheet(
        session,
        sink,
        Point{.x = 200, .y = 320},
        Point{.x = 208, .y = 320},
        MouseButton::Left,
        3);

    EXPECT_EQ(session.state.undoDepth(), 2U);
}

TEST(EditorSinkTest, Handle_RemembersNoStrokeForADragThatPaintedNothing)
{
    Session session;
    EditorSink sink(
        session.state,
        session.overlay,
        session.store,
        session.codec,
        kTranslator);

    press(
        session,
        sink,
        antwika::atlas_editor::widgets::toolWidget(Tool::Pick));

    dragOnSheet(
        session,
        sink,
        Point{.x = 200, .y = 300},
        Point{.x = 208, .y = 300});

    EXPECT_EQ(session.state.undoDepth(), 0U);
}

TEST(EditorSinkTest, Handle_AsksWhatAtlasToMakeFromTheFileMenu)
{
    Session session;
    EditorSink sink(
        session.state,
        session.overlay,
        session.store,
        session.codec,
        kTranslator);

    chooseFile(session, sink, antwika::atlas_editor::FileItem::New, 3);

    EXPECT_EQ(
        session.state.openModal(), antwika::atlas_editor::Modal::New);
    EXPECT_EQ(
        session.state.form(),
        antwika::atlas_editor::formOf(session.state.meta()));
}

TEST(EditorSinkTest, Handle_StopsTheRunFromTheFileMenu)
{
    Session session;
    FakeStopWatcher stopped;
    EditorSink sink(
        session.state,
        session.overlay,
        session.store,
        session.codec,
        kTranslator,
        stopped);

    chooseFile(session, sink, antwika::atlas_editor::FileItem::Quit);

    EXPECT_EQ(stopped.names.size(), 1U);
    EXPECT_EQ(stopped.names.front(), antwika::engine::events::kStop);
}

TEST(EditorSinkTest, Handle_StopsNothingWithNoRunToStop)
{
    Session session;
    EditorSink sink(
        session.state,
        session.overlay,
        session.store,
        session.codec,
        kTranslator);

    chooseFile(session, sink, antwika::atlas_editor::FileItem::Quit);

    EXPECT_EQ(session.state.edits(), 0U);
}

TEST(EditorSinkTest, Handle_ShutsAnOpenMenuOnAPressElsewhere)
{
    namespace widgets = antwika::atlas_editor::widgets;

    Session session;
    EditorSink sink(
        session.state,
        session.overlay,
        session.store,
        session.codec,
        kTranslator);

    press(session, sink, widgets::kFileMenu);

    ASSERT_EQ(
        session.state.openMenu(), antwika::atlas_editor::Menu::File);

    press(session, sink, widgets::swatchWidget(5), 2);

    EXPECT_EQ(
        session.state.openMenu(), antwika::atlas_editor::Menu::None);
    EXPECT_EQ(session.state.colorIndex(), 5U);
}

TEST(EditorSinkTest, Handle_ShutsAnOpenMenuWhenItsOwnButtonIsPressed)
{
    namespace widgets = antwika::atlas_editor::widgets;

    Session session;
    EditorSink sink(
        session.state,
        session.overlay,
        session.store,
        session.codec,
        kTranslator);

    press(session, sink, widgets::kFileMenu);
    press(session, sink, widgets::kFileMenu, 2);

    EXPECT_EQ(
        session.state.openMenu(), antwika::atlas_editor::Menu::None);
}

TEST(EditorSinkTest, Handle_ShutsTheViewMenuFromItsOwnButton)
{
    namespace widgets = antwika::atlas_editor::widgets;

    Session session;
    EditorSink sink(
        session.state,
        session.overlay,
        session.store,
        session.codec,
        kTranslator);

    press(session, sink, widgets::kViewMenu);
    press(session, sink, widgets::kViewMenu, 2);

    EXPECT_EQ(
        session.state.openMenu(), antwika::atlas_editor::Menu::None);
}

TEST(EditorSinkTest, Handle_SwapsOneOpenMenuForTheOther)
{
    namespace widgets = antwika::atlas_editor::widgets;

    Session session;
    EditorSink sink(
        session.state,
        session.overlay,
        session.store,
        session.codec,
        kTranslator);

    press(session, sink, widgets::kFileMenu);
    press(session, sink, widgets::kViewMenu, 2);

    EXPECT_EQ(
        session.state.openMenu(), antwika::atlas_editor::Menu::View);
}

TEST(EditorSinkTest, Handle_IgnoresARepeatedChordAndAnUnmodifiedKey)
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
            .button = MouseButton::Left, .position = {.x = 200, .y = 300}},
        session.codec));

    markOut(
        session,
        sink,
        Point{.x = 200, .y = 300},
        Point{.x = 200, .y = 300});

    sink.handle(inputAt(
        4,
        KeyPressed{
            .key = Key::X,
            .modifiers = {.control = true},
            .repeat = true},
        session.codec));

    sink.handle(inputAt(
        5, KeyPressed{.key = Key::X}, session.codec));

    EXPECT_EQ(
        session.state.image().at(Pixel{.x = 200, .y = 300}),
        defaultPalette().front());
}

TEST(EditorSinkTest, Handle_IgnoresAChordItHasNoUseFor)
{
    Session session;
    EditorSink sink(
        session.state,
        session.overlay,
        session.store,
        session.codec,
        kTranslator);

    chord(session, sink, Key::A);

    EXPECT_EQ(session.state.edits(), 0U);
}

TEST(EditorSinkTest, Handle_PansWithTheMiddleButtonUnderTheSelectTool)
{
    Session session;
    EditorSink sink(
        session.state,
        session.overlay,
        session.store,
        session.codec,
        kTranslator);

    namespace widgets = antwika::atlas_editor::widgets;
    press(session, sink, widgets::toolWidget(Tool::Select));

    const Point before = session.state.view().pan;

    dragOnSheet(
        session,
        sink,
        Point{.x = 200, .y = 300},
        Point{.x = 220, .y = 310},
        MouseButton::Middle,
        4);

    EXPECT_NE(session.state.view().pan, before);
    EXPECT_FALSE(session.state.selection().has_value());
}

TEST(EditorSinkTest, Handle_WalksIntoTheDirectoryAnEntryNames)
{
    namespace widgets = antwika::atlas_editor::widgets;

    const ScratchDirectory dir("atlas_editor_walk_in");
    std::filesystem::create_directories(dir.pathIn("nested"));

    Session session;
    EditorSink sink(
        session.state,
        session.overlay,
        session.store,
        session.codec,
        kTranslator);

    session.state.showModal(
        antwika::atlas_editor::Modal::Load,
        dir.string(),
        antwika::atlas_editor::entriesIn(dir.string()));

    press(session, sink, widgets::fileEntryWidget(1), 2);

    EXPECT_EQ(
        session.state.directory(),
        antwika::atlas_editor::pathIn(dir.string(), "nested"));
    EXPECT_TRUE(session.state.fileName().empty());
}

TEST(EditorSinkTest, Handle_WalksUpOutOfTheParentEntry)
{
    namespace widgets = antwika::atlas_editor::widgets;

    const ScratchDirectory dir("atlas_editor_walk_up");
    std::filesystem::create_directories(dir.pathIn("nested"));

    const auto inner =
        antwika::atlas_editor::pathIn(dir.string(), "nested");

    Session session;
    EditorSink sink(
        session.state,
        session.overlay,
        session.store,
        session.codec,
        kTranslator);

    session.state.showModal(
        antwika::atlas_editor::Modal::Load,
        inner,
        antwika::atlas_editor::entriesIn(inner));

    press(session, sink, widgets::fileEntryWidget(0), 2);

    EXPECT_EQ(session.state.directory(), dir.string());
}

TEST(EditorSinkTest, Handle_SavesIntoTheDirectoryBeingBrowsed)
{
    namespace widgets = antwika::atlas_editor::widgets;

    const ScratchDirectory dir("atlas_editor_save_into");

    Session session;
    EditorSink sink(
        session.state,
        session.overlay,
        session.store,
        session.codec,
        kTranslator);

    session.state.showModal(
        antwika::atlas_editor::Modal::Save,
        dir.string(),
        antwika::atlas_editor::entriesIn(dir.string()));
    session.state.setFileName("sheet.png", antwika::ui::kCaretAtEnd);

    press(session, sink, widgets::kFileConfirm, 2);

    ASSERT_TRUE(session.store.wrote.has_value());
    EXPECT_EQ(
        *session.store.wrote,
        antwika::atlas_editor::pathIn(dir.string(), "sheet.png"));
}

namespace
{
    [[nodiscard]] std::vector<FileEntry> manyFiles(const std::size_t how)
    {
        std::vector<FileEntry> listed;

        for (std::size_t at = 0; at < how; ++at)
        {
            listed.push_back(
                FileEntry{.name = std::to_string(at) + ".png"});
        }

        return listed;
    }

    void wheel(
        Session &session,
        EditorSink &sink,
        const std::int32_t vertical,
        const std::uint64_t tick)
    {
        sink.handle(inputAt(
            tick,
            PointerScrolled{.horizontal = 0, .vertical = vertical},
            session.codec));
    }
}

TEST(EditorSinkTest, Handle_ScrollsTheFileListDownOnAWheelStep)
{
    Session session;
    EditorSink sink(
        session.state,
        session.overlay,
        session.store,
        session.codec,
        kTranslator);

    session.state.showModal(
        antwika::atlas_editor::Modal::Load,
        ".",
        manyFiles(kFileRows + 4));

    wheel(session, sink, -1, 2);

    EXPECT_EQ(session.state.fileScroll(), 1U);
}

TEST(EditorSinkTest, Handle_ScrollsNoFurtherThanTheLastPage)
{
    Session session;
    EditorSink sink(
        session.state,
        session.overlay,
        session.store,
        session.codec,
        kTranslator);

    session.state.showModal(
        antwika::atlas_editor::Modal::Load,
        ".",
        manyFiles(kFileRows + 2));

    for (std::uint64_t step = 0; step < 9; ++step)
    {
        wheel(session, sink, -1, step + 2);
    }

    EXPECT_EQ(session.state.fileScroll(), 2U);
}

TEST(EditorSinkTest, Handle_ScrollsNoFurtherBackThanTheFirstEntry)
{
    Session session;
    EditorSink sink(
        session.state,
        session.overlay,
        session.store,
        session.codec,
        kTranslator);

    session.state.showModal(
        antwika::atlas_editor::Modal::Load,
        ".",
        manyFiles(kFileRows + 4));

    wheel(session, sink, 1, 2);

    EXPECT_EQ(session.state.fileScroll(), 0U);
}

TEST(EditorSinkTest, Handle_KeepsAFileListThatFitsWhereItIs)
{
    Session session;
    EditorSink sink(
        session.state,
        session.overlay,
        session.store,
        session.codec,
        kTranslator);

    session.state.showModal(
        antwika::atlas_editor::Modal::Load,
        ".",
        manyFiles(kFileRows));

    wheel(session, sink, -1, 2);

    EXPECT_EQ(session.state.fileScroll(), 0U);
}

TEST(EditorSinkTest, Handle_LeavesTheFileListAloneWithNoModalOpen)
{
    Session session;
    EditorSink sink(
        session.state,
        session.overlay,
        session.store,
        session.codec,
        kTranslator);

    wheel(session, sink, -1, 2);

    EXPECT_EQ(session.state.fileScroll(), 0U);
}

TEST(EditorSinkTest, Handle_ShowsTheEntriesTheScrollHasReached)
{
    namespace widgets = antwika::atlas_editor::widgets;

    Session session;
    EditorSink sink(
        session.state,
        session.overlay,
        session.store,
        session.codec,
        kTranslator);

    session.state.showModal(
        antwika::atlas_editor::Modal::Load,
        ".",
        manyFiles(kFileRows + 4));

    wheel(session, sink, -1, 2);

    const auto frame =
        describeEditor(session.state, Pointer{}, kTranslator);

    EXPECT_FALSE(
        frame.rects.find(widgets::fileEntryWidget(0)).has_value());
    EXPECT_TRUE(
        frame.rects.find(widgets::fileEntryWidget(1)).has_value());
    EXPECT_TRUE(
        frame.rects.find(widgets::fileEntryWidget(kFileRows))
            .has_value());
}

TEST(EditorSinkTest, Handle_KeepsTheFileListStillOnAHorizontalWheel)
{
    Session session;
    EditorSink sink(
        session.state,
        session.overlay,
        session.store,
        session.codec,
        kTranslator);

    session.state.showModal(
        antwika::atlas_editor::Modal::Load,
        ".",
        manyFiles(kFileRows + 4));

    sink.handle(inputAt(
        2,
        PointerScrolled{.horizontal = 2, .vertical = 0},
        session.codec));

    EXPECT_EQ(session.state.fileScroll(), 0U);
}

TEST(EditorSinkTest, Browse_PutsTheFileListBackToItsFirstEntry)
{
    Session session;

    session.state.showModal(
        antwika::atlas_editor::Modal::Load,
        ".",
        manyFiles(kFileRows + 4));
    session.state.scrollFiles(3);

    ASSERT_EQ(session.state.fileScroll(), 3U);

    session.state.browse("elsewhere", manyFiles(2));

    EXPECT_EQ(session.state.fileScroll(), 0U);
}

TEST(EditorSinkTest, Handle_OpensTheInkPickerFromTheBar)
{
    namespace widgets = antwika::atlas_editor::widgets;

    Session session;
    EditorSink sink(
        session.state,
        session.overlay,
        session.store,
        session.codec,
        kTranslator);

    ASSERT_FALSE(session.state.inkVisible());

    press(session, sink, widgets::kInkButton, 2);

    EXPECT_TRUE(session.state.inkVisible());
}

TEST(EditorSinkTest, Handle_ShutsTheInkPickerOnASecondPress)
{
    namespace widgets = antwika::atlas_editor::widgets;

    Session session;
    EditorSink sink(
        session.state,
        session.overlay,
        session.store,
        session.codec,
        kTranslator);

    press(session, sink, widgets::kInkButton, 2);
    press(session, sink, widgets::kInkButton, 3);

    EXPECT_FALSE(session.state.inkVisible());
}

TEST(EditorSinkTest, Handle_PaintsWithTheChannelASliderWasDraggedTo)
{
    namespace widgets = antwika::atlas_editor::widgets;

    Session session;
    EditorSink sink(
        session.state,
        session.overlay,
        session.store,
        session.codec,
        kTranslator);

    session.state.toggleInk();

    const auto track =
        describeEditor(session.state, Pointer{}, kTranslator)
            .rects.find(widgets::inkChannelWidget(0));
    ASSERT_TRUE(track.has_value());

    const Point at{
        .x = track->origin.x
             + static_cast<std::int32_t>(track->size.width) - 1,
        .y = track->origin.y
             + static_cast<std::int32_t>(track->size.height / 2)};

    sink.handle(inputAt(
        2, PointerMoved{.position = {.x = at.x, .y = at.y}},
        session.codec));
    sink.handle(inputAt(
        2,
        PointerButtonPressed{
            .button = MouseButton::Left,
            .position = {.x = at.x, .y = at.y}},
        session.codec));

    EXPECT_EQ(255, session.state.color().red);
    EXPECT_FALSE(session.state.colorIndex().has_value());
}

TEST(EditorSinkTest, Handle_ForgetsTheInkDragOnTheButtonComingUp)
{
    namespace widgets = antwika::atlas_editor::widgets;

    Session session;
    EditorSink sink(
        session.state,
        session.overlay,
        session.store,
        session.codec,
        kTranslator);

    session.state.toggleInk();
    session.state.setInkDrag(1);

    sink.handle(inputAt(
        2,
        PointerButtonReleased{
            .button = MouseButton::Left,
            .position = {.x = 1, .y = 1}},
        session.codec));

    EXPECT_FALSE(session.state.inkDrag().has_value());
}

TEST(EditorSinkTest, DescribeEditor_ShowsTheInkPickerOnlyWhenItIsOpen)
{
    namespace widgets = antwika::atlas_editor::widgets;

    Session session;

    EXPECT_FALSE(
        describeEditor(session.state, Pointer{}, kTranslator)
            .rects.find(widgets::kInkSwatch)
            .has_value());

    session.state.toggleInk();

    const auto frame =
        describeEditor(session.state, Pointer{}, kTranslator);

    EXPECT_TRUE(
        frame.rects.find(widgets::kInkSwatch).has_value());

    for (std::size_t channel = 0;
         channel < antwika::atlas_editor::kInkChannels;
         ++channel)
    {
        EXPECT_TRUE(
            frame.rects.find(widgets::inkChannelWidget(channel))
                .has_value())
            << channel;
    }
}

TEST(EditorSinkTest, Handle_PaintsWithALaterChannelsSliderToo)
{
    namespace widgets = antwika::atlas_editor::widgets;

    Session session;
    EditorSink sink(
        session.state,
        session.overlay,
        session.store,
        session.codec,
        kTranslator);

    session.state.toggleInk();

    const auto track =
        describeEditor(session.state, Pointer{}, kTranslator)
            .rects.find(widgets::inkChannelWidget(2));
    ASSERT_TRUE(track.has_value());

    const Point at{
        .x = track->origin.x
             + static_cast<std::int32_t>(track->size.width) - 1,
        .y = track->origin.y
             + static_cast<std::int32_t>(track->size.height / 2)};

    sink.handle(inputAt(
        2, PointerMoved{.position = {.x = at.x, .y = at.y}},
        session.codec));
    sink.handle(inputAt(
        2,
        PointerButtonPressed{
            .button = MouseButton::Left,
            .position = {.x = at.x, .y = at.y}},
        session.codec));

    EXPECT_EQ(255, session.state.color().blue);
}

TEST(EditorSinkTest, Handle_LeavesTheInkAloneForASliderThatIsNotOne)
{
    namespace widgets = antwika::atlas_editor::widgets;

    Session session;
    EditorSink sink(
        session.state,
        session.overlay,
        session.store,
        session.codec,
        kTranslator);

    const auto before = session.state.color();

    press(session, sink, widgets::kFileMenu, 2);

    EXPECT_EQ(before, session.state.color());
}

TEST(EditorSinkTest, Handle_LaysOutTheAtlasTheNewCardWasFilledIn)
{
    namespace widgets = antwika::atlas_editor::widgets;
    using antwika::atlas_editor::AtlasField;

    Session session;
    EditorSink sink(
        session.state,
        session.overlay,
        session.store,
        session.codec,
        kTranslator);

    chooseFile(session, sink, antwika::atlas_editor::FileItem::New, 1);

    fillField(session, sink, AtlasField::SpriteWidth, "32");
    fillField(session, sink, AtlasField::SpriteHeight, "16");
    fillField(session, sink, AtlasField::Columns, "3");
    fillField(session, sink, AtlasField::Rows, "2");
    fillField(session, sink, AtlasField::PivotX, "16");
    fillField(session, sink, AtlasField::PivotY, "16");
    fillField(session, sink, AtlasField::IsometricWidth, "16");
    fillField(session, sink, AtlasField::IsometricHeight, "8");

    press(session, sink, widgets::kAtlasCreate, 2);

    EXPECT_EQ(
        session.state.openModal(), antwika::atlas_editor::Modal::None);
    EXPECT_EQ(
        session.state.image().size(),
        (Size{.width = 96, .height = 32}));
    EXPECT_EQ(session.state.meta().columns, 3U);
    EXPECT_EQ(session.state.meta().rows, 2U);
    EXPECT_EQ(
        session.state.tiles(), (TileGrid{.width = 32, .height = 16}));
}

TEST(EditorSinkTest, Handle_RefusesAnAtlasWithNoSlotsInIt)
{
    namespace widgets = antwika::atlas_editor::widgets;
    using antwika::atlas_editor::AtlasField;

    Session session;
    EditorSink sink(
        session.state,
        session.overlay,
        session.store,
        session.codec,
        kTranslator);

    const auto before = session.state.image().size();

    chooseFile(session, sink, antwika::atlas_editor::FileItem::New, 1);
    fillField(session, sink, AtlasField::Columns, "0");
    press(session, sink, widgets::kAtlasCreate, 2);

    EXPECT_EQ(
        session.state.openModal(), antwika::atlas_editor::Modal::New);
    EXPECT_EQ(session.state.image().size(), before);
    ASSERT_TRUE(session.state.status().has_value());
    EXPECT_EQ(
        session.state.status()->id, MessageId::AtlasTooSmall);
}

TEST(EditorSinkTest, Handle_TurnsTheAtlasKindFromTheNewCard)
{
    namespace widgets = antwika::atlas_editor::widgets;

    Session session;
    EditorSink sink(
        session.state,
        session.overlay,
        session.store,
        session.codec,
        kTranslator);

    chooseFile(session, sink, antwika::atlas_editor::FileItem::New, 1);

    const auto before = session.state.form().kind;

    press(session, sink, widgets::kAtlasKind, 2);

    EXPECT_NE(session.state.form().kind, before);
}

TEST(EditorSinkTest, Handle_ShutsTheNewCardWithoutLayingAnAtlasOut)
{
    namespace widgets = antwika::atlas_editor::widgets;

    Session session;
    EditorSink sink(
        session.state,
        session.overlay,
        session.store,
        session.codec,
        kTranslator);

    const auto before = session.state.image().size();

    chooseFile(session, sink, antwika::atlas_editor::FileItem::New, 1);
    press(session, sink, widgets::kFileClose, 2);

    EXPECT_EQ(
        session.state.openModal(), antwika::atlas_editor::Modal::None);
    EXPECT_EQ(session.state.image().size(), before);
}

TEST(EditorSinkTest, Handle_MovesTheCaretToTheAtlasFieldItWasClicked)
{
    using antwika::atlas_editor::AtlasField;

    Session session;
    EditorSink sink(
        session.state,
        session.overlay,
        session.store,
        session.codec,
        kTranslator);

    chooseFile(session, sink, antwika::atlas_editor::FileItem::New, 1);
    fillField(session, sink, AtlasField::Rows, "5");

    EXPECT_EQ(
        session.state.formField(),
        static_cast<std::size_t>(AtlasField::Rows));
    EXPECT_EQ(
        session.state.form().values[static_cast<std::size_t>(
            AtlasField::Rows)],
        "5");
}

TEST(EditorSinkTest, Handle_LaysOutTheAtlasWhenAFieldIsSubmitted)
{
    Session session;
    EditorSink sink(
        session.state,
        session.overlay,
        session.store,
        session.codec,
        kTranslator);

    chooseFile(session, sink, antwika::atlas_editor::FileItem::New, 1);

    sink.handle(inputAt(
        2, KeyPressed{.key = Key::Enter}, session.codec));

    EXPECT_EQ(
        session.state.openModal(), antwika::atlas_editor::Modal::None);
}

TEST(EditorSinkTest, SaveTo_RecordsTheAtlasBesideTheImageItWrote)
{
    Session session;
    EditorSink sink(
        session.state,
        session.overlay,
        session.store,
        session.codec,
        kTranslator);

    saveThrough(session, sink, "sheet.png");

    EXPECT_EQ(session.store.metaWrote, "sheet.png");
    EXPECT_EQ(session.store.metaWritten, session.state.meta());
}

TEST(EditorSinkTest, LoadFrom_TakesOnTheAtlasRecordedBesideTheImage)
{
    Session session;
    EditorSink sink(
        session.state,
        session.overlay,
        session.store,
        session.codec,
        kTranslator);

    session.store.available = blankSheet(Size{.width = 96, .height = 32});
    session.store.describes = antwika::atlas_editor::AtlasMeta{
        .kind = antwika::atlas_editor::AtlasKind::Isometric,
        .columns = 99,
        .rows = 99,
        .sprite = {.width = 32, .height = 16},
        .pivot = {.x = 16, .y = 16},
        .isometric = {.width = 16, .height = 8}};

    loadThrough(session, sink, "sheet.png");

    EXPECT_EQ(
        session.state.tiles(), (TileGrid{.width = 32, .height = 16}));
    EXPECT_EQ(session.state.meta().columns, 3U);
    EXPECT_EQ(session.state.meta().rows, 2U);
    EXPECT_EQ(
        session.state.guides()->footprint,
        (Size{.width = 16, .height = 8}));
}

TEST(EditorSinkTest, LoadFrom_KeepsTheAtlasWhereTheImageCarriesNone)
{
    Session session;
    EditorSink sink(
        session.state,
        session.overlay,
        session.store,
        session.codec,
        kTranslator);

    const auto before = session.state.meta().sprite;

    session.store.available = blankSheet(Size{.width = 96, .height = 32});

    loadThrough(session, sink, "sheet.png");

    EXPECT_EQ(session.state.meta().sprite, before);
}

TEST(EditorSinkTest, Handle_TypesIntoTheAtlasFieldTheCaretSitsIn)
{
    using antwika::atlas_editor::AtlasField;

    Session session;
    EditorSink sink(
        session.state,
        session.overlay,
        session.store,
        session.codec,
        kTranslator);

    chooseFile(session, sink, antwika::atlas_editor::FileItem::New, 1);
    session.state.setFormField("", 0);

    sink.handle(inputAt(
        2, KeyPressed{.key = Key::A}, session.codec));

    EXPECT_EQ(
        session.state.openModal(), antwika::atlas_editor::Modal::New);
    EXPECT_EQ(
        session.state.form().values[static_cast<std::size_t>(
            AtlasField::SpriteWidth)],
        "a");
}

TEST(EditorSinkTest, Handle_LeavesTheNewCardAloneOnAPressBesideIt)
{
    Session session;
    EditorSink sink(
        session.state,
        session.overlay,
        session.store,
        session.codec,
        kTranslator);

    chooseFile(session, sink, antwika::atlas_editor::FileItem::New, 1);

    const auto before = session.state.form();

    sink.handle(inputAt(
        2,
        PointerButtonPressed{
            .button = MouseButton::Left, .position = {.x = 0, .y = 0}},
        session.codec));

    EXPECT_EQ(
        session.state.openModal(), antwika::atlas_editor::Modal::New);
    EXPECT_EQ(session.state.form(), before);
}

TEST(EditorSinkTest, Handle_EmptiesTheMarkedPixelsOnTheDeleteKey)
{
    Session session;
    EditorSink sink(
        session.state,
        session.overlay,
        session.store,
        session.codec,
        kTranslator);

    session.state.selectColor(4);
    dragOnSheet(
        session,
        sink,
        Point{.x = 200, .y = 300},
        Point{.x = 203, .y = 300});

    markOut(session, sink, Point{.x = 196, .y = 296},
            Point{.x = 210, .y = 306});

    sink.handle(inputAt(
        6, KeyPressed{.key = Key::Delete}, session.codec));

    EXPECT_EQ(
        session.state.image().at(Pixel{.x = 202, .y = 300}), kClear);
}

TEST(EditorSinkTest, Handle_KeepsTheDeleteKeyUndoableInOneStep)
{
    Session session;
    EditorSink sink(
        session.state,
        session.overlay,
        session.store,
        session.codec,
        kTranslator);

    session.state.selectColor(4);
    dragOnSheet(
        session,
        sink,
        Point{.x = 200, .y = 300},
        Point{.x = 203, .y = 300});

    markOut(session, sink, Point{.x = 196, .y = 296},
            Point{.x = 210, .y = 306});

    sink.handle(inputAt(
        6, KeyPressed{.key = Key::Delete}, session.codec));

    session.state.undo();

    EXPECT_EQ(
        session.state.image().at(Pixel{.x = 202, .y = 300}),
        defaultPalette()[4]);
}

TEST(EditorSinkTest, Handle_LeavesTheSheetAloneOnDeleteWithAModalOpen)
{
    Session session;
    EditorSink sink(
        session.state,
        session.overlay,
        session.store,
        session.codec,
        kTranslator);

    session.state.selectColor(4);
    dragOnSheet(
        session,
        sink,
        Point{.x = 200, .y = 300},
        Point{.x = 203, .y = 300});

    markOut(session, sink, Point{.x = 196, .y = 296},
            Point{.x = 210, .y = 306});

    chooseFile(session, sink, antwika::atlas_editor::FileItem::Save, 6);

    sink.handle(inputAt(
        7, KeyPressed{.key = Key::Delete}, session.codec));

    EXPECT_EQ(
        session.state.image().at(Pixel{.x = 202, .y = 300}),
        defaultPalette()[4]);
}

TEST(EditorSinkTest, Handle_ShowsThePivotFromTheViewMenu)
{
    Session session;
    EditorSink sink(
        session.state,
        session.overlay,
        session.store,
        session.codec,
        kTranslator);

    ASSERT_FALSE(session.state.pivotVisible());
    ASSERT_TRUE(session.state.guidesVisible());

    chooseView(session, sink, antwika::atlas_editor::ViewItem::Pivot, 1);

    EXPECT_TRUE(session.state.pivotVisible());
    EXPECT_TRUE(session.state.guidesVisible());
}

TEST(EditorSinkTest, Handle_LeavesThePivotAloneWhenTheGuidesAreTurned)
{
    Session session;
    EditorSink sink(
        session.state,
        session.overlay,
        session.store,
        session.codec,
        kTranslator);

    chooseView(session, sink, antwika::atlas_editor::ViewItem::Pivot, 1);
    chooseView(
        session, sink, antwika::atlas_editor::ViewItem::Guides, 2);

    EXPECT_FALSE(session.state.guidesVisible());
    EXPECT_TRUE(session.state.pivotVisible());
}

TEST(EditorSinkTest, Handle_TurnsThePointerBorderOffFromTheViewMenu)
{
    Session session;
    EditorSink sink(
        session.state,
        session.overlay,
        session.store,
        session.codec,
        kTranslator);

    ASSERT_TRUE(session.state.pointerBorderVisible());

    chooseView(
        session,
        sink,
        antwika::atlas_editor::ViewItem::PointerBorder,
        1);

    EXPECT_FALSE(session.state.pointerBorderVisible());
}

TEST(EditorSinkTest, Handle_StillRulesThePixelGridFromTheViewMenu)
{
    Session session;
    EditorSink sink(
        session.state,
        session.overlay,
        session.store,
        session.codec,
        kTranslator);

    chooseView(
        session, sink, antwika::atlas_editor::ViewItem::PixelGrid, 1);

    EXPECT_TRUE(session.state.pixelGridVisible());
}

TEST(EditorSinkTest, Handle_FillsTheNewCardInFromAPreset)
{
    namespace widgets = antwika::atlas_editor::widgets;

    Session session;
    EditorSink sink(
        session.state,
        session.overlay,
        session.store,
        session.codec,
        kTranslator);

    chooseFile(session, sink, antwika::atlas_editor::FileItem::New, 1);

    press(session, sink, widgets::kAtlasPresets, 2);
    press(session, sink, widgets::atlasPresetWidget(1), 2);

    EXPECT_EQ(
        session.state.form(), antwika::atlas_editor::presetForm(1));
    EXPECT_EQ(
        session.state.openMenu(), antwika::atlas_editor::Menu::None);
}

TEST(EditorSinkTest, Handle_LaysOutTheSheetAPresetAsksFor)
{
    namespace widgets = antwika::atlas_editor::widgets;

    Session session;
    EditorSink sink(
        session.state,
        session.overlay,
        session.store,
        session.codec,
        kTranslator);

    chooseFile(session, sink, antwika::atlas_editor::FileItem::New, 1);

    press(session, sink, widgets::kAtlasPresets, 2);
    press(session, sink, widgets::atlasPresetWidget(1), 2);
    press(session, sink, widgets::kAtlasCreate, 3);

    EXPECT_EQ(
        session.state.image().size(),
        (Size{.width = 768, .height = 896}));
    EXPECT_EQ(
        session.state.tiles(), (TileGrid{.width = 96, .height = 112}));
    EXPECT_EQ(
        session.state.meta().pivot, (Point{.x = 48, .y = 80}));
    EXPECT_EQ(
        session.state.meta().isometric,
        (Size{.width = 64, .height = 32}));
}

TEST(EditorSinkTest, Handle_ShutsThePresetListWithoutTakingOne)
{
    namespace widgets = antwika::atlas_editor::widgets;

    Session session;
    EditorSink sink(
        session.state,
        session.overlay,
        session.store,
        session.codec,
        kTranslator);

    chooseFile(session, sink, antwika::atlas_editor::FileItem::New, 1);

    const auto before = session.state.form();

    press(session, sink, widgets::kAtlasPresets, 2);
    press(session, sink, widgets::kAtlasPresets, 3);

    EXPECT_EQ(
        session.state.openMenu(), antwika::atlas_editor::Menu::None);
    EXPECT_EQ(session.state.form(), before);
}
