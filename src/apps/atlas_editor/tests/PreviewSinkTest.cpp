#include <gtest/gtest.h>

#include <cstdint>
#include <optional>
#include <string>

#include <antwika/engine/Events.hpp>
#include <antwika/event/Event.hpp>
#include <antwika/event/TickEvent.hpp>
#include <antwika/gfx/Bitmap.hpp>
#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/Rect.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/i18n/Locale.hpp>
#include <antwika/input/InputEvent.hpp>
#include <antwika/input/InputEventCodec.hpp>
#include <antwika/input/Key.hpp>
#include <antwika/input/MouseButton.hpp>

#include <antwika/ui/Pointer.hpp>

#include "antwika/atlas_editor/AtlasMeta.hpp"
#include "antwika/atlas_editor/Canvas.hpp"
#include "antwika/atlas_editor/EditorSink.hpp"
#include "antwika/atlas_editor/EditorState.hpp"
#include "antwika/atlas_editor/EditorUi.hpp"
#include "antwika/atlas_editor/IAtlasStore.hpp"
#include "antwika/atlas_editor/Messages.hpp"
#include "antwika/atlas_editor/CanvasView.hpp"
#include "antwika/atlas_editor/Pixel.hpp"
#include "antwika/atlas_editor/Preview.hpp"
#include "antwika/atlas_editor/TileGrid.hpp"
#include "antwika/atlas_editor/UiOverlay.hpp"

using antwika::atlas_editor::Canvas;
using antwika::atlas_editor::EditorSink;
using antwika::atlas_editor::EditorState;
using antwika::atlas_editor::IAtlasStore;
using antwika::atlas_editor::TileGrid;
using antwika::atlas_editor::UiOverlay;
using antwika::event::Event;
using antwika::event::TickEvent;
using antwika::gfx::Bitmap;
using antwika::gfx::Point;
using antwika::gfx::Rect;
using antwika::gfx::Size;
using antwika::input::InputEvent;
using antwika::input::InputEventCodec;
using antwika::input::MouseButton;
using antwika::input::PointerButtonPressed;
using antwika::input::PointerButtonReleased;
using antwika::input::PointerMoved;
using antwika::input::PointerScrolled;

namespace
{
    constexpr Size kCanvas{.width = 800, .height = 480};

    constexpr antwika::atlas_editor::Translator kTranslator{
        antwika::i18n::kDefaultLocale};

    class FakeIdleStore final : public IAtlasStore
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

        [[nodiscard]] std::optional<antwika::atlas_editor::AtlasMeta>
        loadMetaFrom(const std::string &) override
        {
            return std::nullopt;
        }

        void saveMetaTo(
            const antwika::atlas_editor::AtlasMeta &,
            const std::string &) override
        {
        }

        [[nodiscard]] std::string savePath() const override
        {
            return "sheet.png";
        }
    };

    struct Session final
    {
        EditorState state{
            Canvas::blank(Size{.width = 64, .height = 64}),
            TileGrid{.width = 16, .height = 16},
            kCanvas};
        UiOverlay overlay{};
        FakeIdleStore store{};
        InputEventCodec codec{};
    };

    [[nodiscard]] TickEvent tickAt(const std::uint64_t tick)
    {
        return TickEvent{
            .tick = tick,
            .event = Event{.name = antwika::engine::events::kTick}};
    }

    [[nodiscard]] TickEvent inputAt(
        const std::uint64_t tick,
        const InputEvent &event,
        const InputEventCodec &codec)
    {
        return TickEvent{.tick = tick, .event = codec.encode(event)};
    }

    [[nodiscard]] Point middleOf(const Rect &rect)
    {
        return Point{
            .x = rect.origin.x
                 + static_cast<std::int32_t>(rect.size.width / 2),
            .y = rect.origin.y
                 + static_cast<std::int32_t>(rect.size.height / 2)};
    }

    void openPreview(Session &session, EditorSink &sink)
    {
        session.state.togglePreview();
        sink.handle(tickAt(1));
    }

    void chooseView(
        Session &session,
        EditorSink &sink,
        const antwika::atlas_editor::ViewItem item)
    {
        session.state.showMenu(antwika::atlas_editor::Menu::View);

        const auto option =
            antwika::atlas_editor::describeEditor(
                session.state, antwika::ui::Pointer{}, kTranslator)
                .rects.find(
                    antwika::atlas_editor::widgets::viewItemWidget(item));

        const Point at = middleOf(option.value_or(Rect{}));

        sink.handle(inputAt(
            1,
            PointerMoved{.position = {.x = at.x, .y = at.y}},
            session.codec));
        sink.handle(inputAt(
            1,
            PointerButtonPressed{
                .button = MouseButton::Left,
                .position = {.x = at.x, .y = at.y}},
            session.codec));
    }

    [[nodiscard]] Rect previewPane(const Session &session)
    {
        return session.overlay.panes().preview.value_or(Rect{});
    }
}

TEST(PreviewSinkTest, Handle_LeavesNoPreviewPaneWhileTheViewIsWhole)
{
    Session session;
    EditorSink sink(
        session.state,
        session.overlay,
        session.store,
        session.codec,
        kTranslator);

    sink.handle(tickAt(1));

    EXPECT_FALSE(session.overlay.panes().preview.has_value());
    EXPECT_NE(session.overlay.panes().sheet, Rect{});
}

TEST(PreviewSinkTest, Handle_SplitsTheViewOnceThePreviewIsOpen)
{
    Session session;
    EditorSink sink(
        session.state,
        session.overlay,
        session.store,
        session.codec,
        kTranslator);

    openPreview(session, sink);

    const auto pane = previewPane(session);
    const auto sheet = session.overlay.panes().sheet;

    EXPECT_NE(pane, Rect{});
    EXPECT_GT(pane.origin.x, sheet.origin.x);
}

TEST(PreviewSinkTest, Handle_FramesTheWholeSheetBeforeAnythingIsTouched)
{
    Session session;
    EditorSink sink(
        session.state,
        session.overlay,
        session.store,
        session.codec,
        kTranslator);

    openPreview(session, sink);

    EXPECT_EQ(
        session.state.preview().view,
        antwika::atlas_editor::fittedView(
            previewPane(session),
            Rect{
                .origin = {},
                .size = session.state.image().size()}));
}

TEST(PreviewSinkTest, Handle_FramesTheSlotTheLastEditLandedIn)
{
    Session session;
    EditorSink sink(
        session.state,
        session.overlay,
        session.store,
        session.codec,
        kTranslator);

    openPreview(session, sink);

    session.state.noteTouched(
        antwika::atlas_editor::Pixel{.x = 40, .y = 40});

    sink.handle(tickAt(2));

    ASSERT_TRUE(session.state.preview().focused.has_value());
    EXPECT_EQ(
        session.state.preview().view,
        *antwika::atlas_editor::viewOfSlot(
            previewPane(session),
            session.state.tiles(),
            session.state.image().size(),
            *session.state.preview().focused));
}

TEST(PreviewSinkTest, Handle_ZoomsThePreviewWhenTheWheelTurnsOverIt)
{
    Session session;
    EditorSink sink(
        session.state,
        session.overlay,
        session.store,
        session.codec,
        kTranslator);

    openPreview(session, sink);

    const auto sheetZoom = session.state.view().zoom;
    const Point at = middleOf(previewPane(session));

    sink.handle(inputAt(
        2,
        PointerMoved{.position = {.x = at.x, .y = at.y}},
        session.codec));

    const auto was = session.state.preview().view.zoom;

    sink.handle(inputAt(
        2, PointerScrolled{.vertical = 1}, session.codec));

    EXPECT_EQ(session.state.preview().view.zoom, was + 1);
    EXPECT_EQ(session.state.view().zoom, sheetZoom);
}

TEST(PreviewSinkTest, Handle_ZoomsThePreviewOutWhenTheWheelTurnsBack)
{
    Session session;
    EditorSink sink(
        session.state,
        session.overlay,
        session.store,
        session.codec,
        kTranslator);

    openPreview(session, sink);

    const Point at = middleOf(previewPane(session));

    sink.handle(inputAt(
        2,
        PointerMoved{.position = {.x = at.x, .y = at.y}},
        session.codec));
    sink.handle(inputAt(
        2, PointerScrolled{.vertical = 1}, session.codec));

    const auto was = session.state.preview().view.zoom;

    sink.handle(inputAt(
        3, PointerScrolled{.vertical = -1}, session.codec));

    EXPECT_EQ(session.state.preview().view.zoom, was - 1);
}

TEST(PreviewSinkTest, Handle_TurnsAutoFocusOffWhenThePaneIsZoomedByHand)
{
    Session session;
    EditorSink sink(
        session.state,
        session.overlay,
        session.store,
        session.codec,
        kTranslator);

    openPreview(session, sink);

    ASSERT_TRUE(session.state.preview().autoFocus);

    const Point at = middleOf(previewPane(session));

    sink.handle(inputAt(
        2,
        PointerMoved{.position = {.x = at.x, .y = at.y}},
        session.codec));
    sink.handle(inputAt(
        2, PointerScrolled{.vertical = 1}, session.codec));

    EXPECT_FALSE(session.state.preview().autoFocus);
}

TEST(PreviewSinkTest, Handle_PaintsNoPixelThroughThePreviewPane)
{
    Session session;
    EditorSink sink(
        session.state,
        session.overlay,
        session.store,
        session.codec,
        kTranslator);

    openPreview(session, sink);

    const auto before = session.state.edits();
    const Point at = middleOf(previewPane(session));

    sink.handle(inputAt(
        2,
        PointerButtonPressed{
            .button = MouseButton::Left, .position = {.x = at.x, .y = at.y}},
        session.codec));

    EXPECT_EQ(session.state.edits(), before);
}

TEST(PreviewSinkTest, Handle_KeepsADragThatBeganInThePaneOffTheSheet)
{
    Session session;
    EditorSink sink(
        session.state,
        session.overlay,
        session.store,
        session.codec,
        kTranslator);

    openPreview(session, sink);

    const auto before = session.state.edits();
    const Point at = middleOf(previewPane(session));
    const Point sheet = middleOf(session.overlay.panes().sheet);

    sink.handle(inputAt(
        2,
        PointerButtonPressed{
            .button = MouseButton::Left, .position = {.x = at.x, .y = at.y}},
        session.codec));

    sink.handle(inputAt(
        3,
        PointerMoved{.position = {.x = sheet.x, .y = sheet.y}},
        session.codec));

    EXPECT_EQ(session.state.edits(), before);
}

TEST(PreviewSinkTest, Handle_PaintsAgainOnceThePreviewDragIsReleased)
{
    Session session;
    EditorSink sink(
        session.state,
        session.overlay,
        session.store,
        session.codec,
        kTranslator);

    openPreview(session, sink);

    const Point at = middleOf(previewPane(session));
    const Point sheet = middleOf(
        antwika::atlas_editor::pixelRect(
            session.state.view(),
            antwika::atlas_editor::Pixel{.x = 32, .y = 32}));

    sink.handle(inputAt(
        2,
        PointerButtonPressed{
            .button = MouseButton::Left, .position = {.x = at.x, .y = at.y}},
        session.codec));

    sink.handle(inputAt(
        3,
        PointerButtonReleased{
            .button = MouseButton::Left, .position = {.x = at.x, .y = at.y}},
        session.codec));

    const auto before = session.state.edits();

    sink.handle(inputAt(
        4,
        PointerMoved{.position = {.x = sheet.x, .y = sheet.y}},
        session.codec));
    sink.handle(inputAt(
        4,
        PointerButtonPressed{
            .button = MouseButton::Left,
            .position = {.x = sheet.x, .y = sheet.y}},
        session.codec));

    EXPECT_GT(session.state.edits(), before);
}

TEST(PreviewSinkTest, Handle_SlidesThePaneWhileTheDragIsOnIt)
{
    Session session;
    EditorSink sink(
        session.state,
        session.overlay,
        session.store,
        session.codec,
        kTranslator);

    openPreview(session, sink);

    const Point at = middleOf(previewPane(session));
    const auto was = session.state.preview().view.pan;

    sink.handle(inputAt(
        2,
        PointerButtonPressed{
            .button = MouseButton::Left, .position = {.x = at.x, .y = at.y}},
        session.codec));
    sink.handle(inputAt(
        3,
        PointerMoved{.position = {.x = at.x + 10, .y = at.y + 6}},
        session.codec));

    EXPECT_NE(session.state.preview().view.pan, was);
}

TEST(PreviewSinkTest, Handle_MovesTheDividerWhenItIsDragged)
{
    Session session;
    EditorSink sink(
        session.state,
        session.overlay,
        session.store,
        session.codec,
        kTranslator);

    openPreview(session, sink);

    const auto was = session.state.preview().ratio;

    const auto divider =
        antwika::atlas_editor::describeEditor(
            session.state, antwika::ui::Pointer{}, kTranslator)
            .rects.find(antwika::atlas_editor::widgets::kPreviewDivider);

    ASSERT_TRUE(divider.has_value());

    const Point at = middleOf(*divider);

    sink.handle(inputAt(
        2,
        PointerMoved{.position = {.x = at.x - 80, .y = at.y}},
        session.codec));
    sink.handle(inputAt(
        2,
        PointerButtonPressed{
            .button = MouseButton::Left,
            .position = {.x = at.x, .y = at.y}},
        session.codec));
    sink.handle(inputAt(
        3,
        PointerMoved{.position = {.x = at.x - 80, .y = at.y}},
        session.codec));

    EXPECT_LT(session.state.preview().ratio, was);
}

TEST(PreviewSinkTest, Handle_OpensThePreviewFromTheViewMenu)
{
    Session session;
    EditorSink sink(
        session.state,
        session.overlay,
        session.store,
        session.codec,
        kTranslator);

    chooseView(
        session, sink, antwika::atlas_editor::ViewItem::Preview);

    EXPECT_TRUE(session.state.preview().open);
}

TEST(PreviewSinkTest, Handle_TurnsAutoFocusOffFromTheViewMenu)
{
    Session session;
    EditorSink sink(
        session.state,
        session.overlay,
        session.store,
        session.codec,
        kTranslator);

    chooseView(
        session, sink, antwika::atlas_editor::ViewItem::PreviewFocus);

    EXPECT_FALSE(session.state.preview().autoFocus);
}

TEST(PreviewSinkTest, Handle_LeavesThePaneWhereItIsOnAFlatScroll)
{
    Session session;
    EditorSink sink(
        session.state,
        session.overlay,
        session.store,
        session.codec,
        kTranslator);

    openPreview(session, sink);

    const Point at = middleOf(previewPane(session));

    sink.handle(inputAt(
        2,
        PointerMoved{.position = {.x = at.x, .y = at.y}},
        session.codec));

    const auto was = session.state.preview().view;

    sink.handle(inputAt(
        3, PointerScrolled{.vertical = 0}, session.codec));

    EXPECT_EQ(session.state.preview().view, was);
}

TEST(PreviewSinkTest, Handle_SlidesNothingForAKeyPressedMidDrag)
{
    Session session;
    EditorSink sink(
        session.state,
        session.overlay,
        session.store,
        session.codec,
        kTranslator);

    openPreview(session, sink);

    const Point at = middleOf(previewPane(session));

    sink.handle(inputAt(
        2,
        PointerButtonPressed{
            .button = MouseButton::Left, .position = {.x = at.x, .y = at.y}},
        session.codec));

    const auto was = session.state.preview().view;

    sink.handle(inputAt(
        3,
        antwika::input::KeyPressed{.key = antwika::input::Key::A},
        session.codec));

    EXPECT_EQ(session.state.preview().view, was);
}

TEST(PreviewSinkTest, Handle_FramesNothingWhenTheSlotLeavesTheGrid)
{
    Session session;
    EditorSink sink(
        session.state,
        session.overlay,
        session.store,
        session.codec,
        kTranslator);

    openPreview(session, sink);

    session.state.noteTouched(
        antwika::atlas_editor::Pixel{.x = 48, .y = 48});

    sink.handle(tickAt(2));

    const auto framed = session.state.preview().view;

    session.state.adoptMeta(antwika::atlas_editor::AtlasMeta{
        .columns = 1,
        .rows = 1,
        .sprite = {.width = 64, .height = 64}});

    sink.handle(tickAt(3));

    EXPECT_EQ(session.state.preview().view, framed);
}
