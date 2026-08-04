#include <chrono>
#include <memory>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <antwika/engine/Events.hpp>
#include <antwika/event/Event.hpp>
#include <antwika/event/TickEvent.hpp>
#include <antwika/gfx/Bitmap.hpp>
#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/mocks/MockRenderer.hpp>
#include <antwika/gfx/mocks/MockTexture.hpp>
#include <antwika/gfx/mocks/MockWindow.hpp>
#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/Rect.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/time/fakes/FakeSleeper.hpp>

#include "antwika/atlas_editor/Canvas.hpp"
#include "antwika/atlas_editor/CanvasView.hpp"
#include "antwika/atlas_editor/EditorScene.hpp"
#include "antwika/atlas_editor/EditorState.hpp"
#include "antwika/atlas_editor/Pixel.hpp"
#include <antwika/console/ConsolePicture.hpp>

#include "antwika/atlas_editor/RenderSink.hpp"
#include "antwika/atlas_editor/SceneSnapshot.hpp"
#include "antwika/atlas_editor/Selection.hpp"
#include "antwika/atlas_editor/SpriteGuides.hpp"
#include "antwika/atlas_editor/TileGrid.hpp"
#include "antwika/atlas_editor/Tool.hpp"
#include "antwika/atlas_editor/UiOverlay.hpp"

using antwika::atlas_editor::Canvas;
using antwika::atlas_editor::CanvasView;
using antwika::atlas_editor::EditorScene;
using antwika::atlas_editor::EditorState;
using antwika::atlas_editor::imageRect;
using antwika::atlas_editor::Pixel;
using antwika::atlas_editor::pixelRect;
using antwika::atlas_editor::RenderSink;
using antwika::atlas_editor::SceneSnapshot;
using antwika::atlas_editor::Selection;
using antwika::atlas_editor::snapshotOf;
using antwika::atlas_editor::SpriteGuides;
using antwika::atlas_editor::TileGrid;
using antwika::atlas_editor::UiOverlay;
using antwika::event::Event;
using antwika::event::TickEvent;
using antwika::gfx::Bitmap;
using antwika::gfx::mocks::MockRenderer;
using antwika::gfx::mocks::MockTexture;
using antwika::gfx::mocks::MockWindow;
using antwika::gfx::Point;
using antwika::gfx::Rect;
using antwika::gfx::Size;
using antwika::time::fakes::FakeSleeper;
using ::testing::_;
using ::testing::AnyNumber;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::ReturnRef;

namespace
{
    constexpr Size kCanvas{.width = 200, .height = 120};
    constexpr Size kSheet{.width = 16, .height = 8};
    constexpr std::chrono::milliseconds kFramePeriod{40};

    SceneSnapshot snapshot()
    {
        return SceneSnapshot{
            .image = kSheet,
            .view = CanvasView{.pan = {.x = 10, .y = 20}, .zoom = 1},
            .tiles = TileGrid{.width = 8, .height = 4},
            .gridVisible = false,
            .hovered = std::nullopt};
    }

    TickEvent tickAt(const std::uint64_t tick)
    {
        return TickEvent{
            .tick = tick,
            .event = Event{.name = antwika::engine::events::kTick}};
    }

    EditorState opened()
    {
        return EditorState{Canvas::blank(kSheet), TileGrid{}, kCanvas};
    }

    // A sheet of two of the game's 1x1 slots.
    // Drawn one to one from the canvas's own corner.
    // So a guide's canvas position is its position in the sheet.
    // Which leaves the arithmetic below as the contract's own numbers.
    constexpr Size kGuidedSheet{.width = 128, .height = 96};
    constexpr TileGrid kGameSlots{.width = 64, .height = 96};
    constexpr SpriteGuides kGameSlotGuides{
        .pivot = {.x = 32, .y = 64},
        .footprint = {.width = 32, .height = 16}};

    SceneSnapshot guided(const std::optional<SpriteGuides> guides)
    {
        return SceneSnapshot{
            .image = kGuidedSheet,
            .view = CanvasView{.pan = {}, .zoom = 0},
            .tiles = kGameSlots,
            .gridVisible = false,
            .guides = guides,
            .hovered = std::nullopt};
    }
} // namespace

TEST(EditorSceneTest, Draw_ClearsAndBacksTheSheetBeforeAnythingElse)
{
    NiceMock<MockRenderer> renderer;
    const EditorScene scene;
    const auto shown = snapshot();

    EXPECT_CALL(renderer, clear(_)).Times(1);
    EXPECT_CALL(
        renderer, drawRect(imageRect(shown.view, shown.image), _))
        .Times(1);

    scene.draw(renderer, shown, nullptr);
}

TEST(EditorSceneTest, Draw_BlitsTheWholeSheetWhenThereIsATextureOfIt)
{
    NiceMock<MockRenderer> renderer;
    NiceMock<MockTexture> texture;
    const EditorScene scene;
    const auto shown = snapshot();

    EXPECT_CALL(
        renderer,
        drawTexture(
            _,
            Rect{.origin = {}, .size = kSheet},
            imageRect(shown.view, shown.image),
            _))
        .Times(1);

    scene.draw(renderer, shown, &texture);
}

TEST(EditorSceneTest, Draw_DrawsNoLinesWhileTheGridIsHidden)
{
    NiceMock<MockRenderer> renderer;
    const EditorScene scene;

    EXPECT_CALL(renderer, drawLine(_, _, _)).Times(0);

    scene.draw(renderer, snapshot(), nullptr);
}

TEST(EditorSceneTest, Draw_DrawsOneLinePerSlotBoundaryOnBothAxes)
{
    NiceMock<MockRenderer> renderer;
    const EditorScene scene;

    auto shown = snapshot();
    shown.gridVisible = true;

    // Sixteen by eight pixels in slots of eight by four.
    // Two columns and two rows, so three lines on each axis.
    EXPECT_CALL(renderer, drawLine(_, _, _)).Times(6);

    scene.draw(renderer, shown, nullptr);
}

TEST(EditorSceneTest, Draw_DrawsNoDiamondsWhileTheGuidesAreOff)
{
    NiceMock<MockRenderer> renderer;
    const EditorScene scene;

    // The grid is hidden too, so a line drawn is a guide's.
    EXPECT_CALL(renderer, drawLine(_, _, _)).Times(0);

    scene.draw(renderer, guided(std::nullopt), nullptr);
}

TEST(EditorSceneTest, Draw_DrawsADiamondAndAPivotInEveryWholeSlot)
{
    NiceMock<MockRenderer> renderer;
    const EditorScene scene;

    // Two slots, four diamond sides and a two-armed cross each.
    EXPECT_CALL(renderer, drawLine(_, _, _)).Times(12);

    scene.draw(renderer, guided(kGameSlotGuides), nullptr);
}

// The corners, said out loud rather than counted.
// Where a diamond is drawn is the whole of what these guides are for.
// A count would pass just as happily with them all in one place.
TEST(EditorSceneTest, Draw_PutsADiamondsCornersRoundItsSlotsPivot)
{
    NiceMock<MockRenderer> renderer;
    const EditorScene scene;

    constexpr Point kTop{.x = 32, .y = 48};
    constexpr Point kRight{.x = 48, .y = 56};
    constexpr Point kBottom{.x = 32, .y = 64};
    constexpr Point kLeft{.x = 16, .y = 56};

    EXPECT_CALL(renderer, drawLine(_, _, _)).Times(AnyNumber());
    EXPECT_CALL(renderer, drawLine(kTop, kRight, _)).Times(1);
    EXPECT_CALL(renderer, drawLine(kRight, kBottom, _)).Times(1);
    EXPECT_CALL(renderer, drawLine(kBottom, kLeft, _)).Times(1);
    EXPECT_CALL(renderer, drawLine(kLeft, kTop, _)).Times(1);

    // The cross is centred on the pivot, which is the bottom corner.
    EXPECT_CALL(
        renderer,
        drawLine(Point{.x = 30, .y = 64}, Point{.x = 34, .y = 64}, _))
        .Times(1);
    EXPECT_CALL(
        renderer,
        drawLine(Point{.x = 32, .y = 62}, Point{.x = 32, .y = 66}, _))
        .Times(1);

    scene.draw(renderer, guided(kGameSlotGuides), nullptr);
}

// Every guide is drawn through the view, exactly as the sheet is.
// A guide ignoring the zoom would sit over the art at one level only.
// Which is the one bug an overlay like this must not have.
TEST(EditorSceneTest, Draw_ScalesAndPansTheDiamondsWithTheSheet)
{
    NiceMock<MockRenderer> renderer;
    const EditorScene scene;

    auto shown = guided(kGameSlotGuides);
    shown.view = CanvasView{.pan = {.x = 5, .y = 7}, .zoom = 1};

    // Twice the size, from a corner five across and seven down.
    // The diamond's left corner is at (16, 56) in the sheet.
    // Its top corner is at (32, 48).
    // So the side joining the two runs between these.
    EXPECT_CALL(renderer, drawLine(_, _, _)).Times(AnyNumber());
    EXPECT_CALL(
        renderer,
        drawLine(
            Point{.x = (16 * 2) + 5, .y = (56 * 2) + 7},
            Point{.x = (32 * 2) + 5, .y = (48 * 2) + 7},
            _))
        .Times(1);

    scene.draw(renderer, shown, nullptr);
}

TEST(EditorSceneTest, Draw_OutlinesTheMarkedRectangleRoundItsPixels)
{
    NiceMock<MockRenderer> renderer;
    const EditorScene scene;

    auto shown = guided(std::nullopt);
    shown.selection = Selection{
        .origin = {.x = 2, .y = 1}, .size = {.width = 3, .height = 2}};

    // Round the outside, so the far edge is one past the last pixel.
    constexpr Point kCorner{.x = 2, .y = 1};
    constexpr Point kFar{.x = 5, .y = 3};

    EXPECT_CALL(renderer, drawLine(_, _, _)).Times(AnyNumber());
    EXPECT_CALL(
        renderer, drawLine(kCorner, Point{.x = 5, .y = 1}, _)).Times(1);
    EXPECT_CALL(
        renderer, drawLine(Point{.x = 5, .y = 1}, kFar, _)).Times(1);
    EXPECT_CALL(
        renderer, drawLine(kFar, Point{.x = 2, .y = 3}, _)).Times(1);
    EXPECT_CALL(
        renderer, drawLine(Point{.x = 2, .y = 3}, kCorner, _)).Times(1);

    scene.draw(renderer, shown, nullptr);
}

TEST(EditorSceneTest, Draw_ScalesAndPansTheMarkedRectangleToo)
{
    NiceMock<MockRenderer> renderer;
    const EditorScene scene;

    auto shown = guided(std::nullopt);
    shown.view = CanvasView{.pan = {.x = 5, .y = 7}, .zoom = 1};
    shown.selection = Selection{
        .origin = {.x = 2, .y = 1}, .size = {.width = 3, .height = 2}};

    EXPECT_CALL(renderer, drawLine(_, _, _)).Times(AnyNumber());
    EXPECT_CALL(
        renderer,
        drawLine(
            Point{.x = (2 * 2) + 5, .y = (1 * 2) + 7},
            Point{.x = (5 * 2) + 5, .y = (1 * 2) + 7},
            _))
        .Times(1);

    scene.draw(renderer, shown, nullptr);
}

// Four sides and no more, the grid and the guides both being off.
TEST(EditorSceneTest, Draw_DrawsFourSidesForTheMarkedRectangle)
{
    NiceMock<MockRenderer> renderer;
    const EditorScene scene;

    auto shown = guided(std::nullopt);
    shown.selection = Selection{
        .origin = {.x = 2, .y = 1}, .size = {.width = 3, .height = 2}};

    EXPECT_CALL(renderer, drawLine(_, _, _)).Times(4);

    scene.draw(renderer, shown, nullptr);
}

TEST(EditorSceneTest, Draw_OutlinesThePixelUnderThePointer)
{
    NiceMock<MockRenderer> renderer;
    const EditorScene scene;

    auto shown = snapshot();
    shown.hovered = Pixel{.x = 3, .y = 2};

    // The sheet's own backing is a fill too.
    // One expectation on a method makes every call have to match one.
    EXPECT_CALL(renderer, drawRect(_, _)).Times(AnyNumber());
    EXPECT_CALL(
        renderer,
        drawRect(pixelRect(shown.view, Pixel{.x = 3, .y = 2}), _))
        .Times(1);

    scene.draw(renderer, shown, nullptr);
}

TEST(SnapshotOfTest, SnapshotOf_TakesTheDrawingHalfOfTheState)
{
    EditorState state = opened();
    state.toggleGrid();
    state.moveTo(state.view().pan);

    const auto shown = snapshotOf(state);

    EXPECT_EQ(shown.image, kSheet);
    EXPECT_EQ(shown.view, state.view());
    EXPECT_EQ(shown.tiles, state.tiles());
    EXPECT_FALSE(shown.gridVisible);
    EXPECT_EQ(shown.hovered, (Pixel{.x = 0, .y = 0}));
    EXPECT_EQ(shown.guides, state.guides());
    EXPECT_FALSE(shown.selection.has_value());
}

// The one a drag is heading for rather than the one it started from.
// So the outline follows the pointer and the sheet waits for the button.
TEST(SnapshotOfTest, SnapshotOf_CarriesTheSelectionADragIsHeadingFor)
{
    EditorState state = opened();
    state.selectTool(antwika::atlas_editor::Tool::Select);

    state.beginSelecting(state.view().pan);
    state.dragSelectionTo(
        Point{.x = state.view().pan.x + 3, .y = state.view().pan.y + 2});

    const auto shown = snapshotOf(state);

    ASSERT_TRUE(shown.selection.has_value());
    EXPECT_EQ(shown.selection->size, (Size{.width = 4, .height = 3}));
}

// Hidden and having none come to the same picture.
// So they come to the same snapshot.
// Which leaves the scene one thing to ask rather than two.
TEST(SnapshotOfTest, SnapshotOf_CarriesNoGuidesWhileTheyAreHidden)
{
    EditorState state = opened();
    ASSERT_TRUE(state.guides().has_value());

    state.toggleGuides();

    EXPECT_FALSE(snapshotOf(state).guides.has_value());
}

TEST(SnapshotOfTest, SnapshotOf_CarriesNoGuidesForASlotWithNone)
{
    const EditorState state{
        Canvas::blank(kSheet),
        TileGrid{.width = 8, .height = 4},
        kCanvas};

    EXPECT_TRUE(state.guidesVisible());
    EXPECT_FALSE(snapshotOf(state).guides.has_value());
}

TEST(RenderSinkTest, Handle_UploadsTheSheetOnceAndAgainWhenItChanges)
{
    NiceMock<MockWindow> window;
    NiceMock<MockRenderer> renderer;
    FakeSleeper sleeper;
    const EditorScene scene;
    EditorState state = opened();
    const UiOverlay overlay;

    ON_CALL(window, isOpen()).WillByDefault(Return(true));
    ON_CALL(window, renderer()).WillByDefault(ReturnRef(renderer));
    ON_CALL(renderer, createTexture(_))
        .WillByDefault(
            [](const Bitmap &)
            { return std::make_unique<NiceMock<MockTexture>>(); });

    EXPECT_CALL(renderer, createTexture(_)).Times(2);
    EXPECT_CALL(renderer, present()).Times(3);

    const antwika::console::ConsolePicture noConsole;
    RenderSink sink(
        window, scene, state, overlay, noConsole, sleeper,
        kFramePeriod);

    sink.handle(tickAt(1));

    // Nothing changed, so nothing is uploaded again.
    sink.handle(tickAt(2));

    state.applyAt(state.view().pan);
    sink.handle(tickAt(3));

    EXPECT_EQ(sleeper.requested().size(), 3U);
}

// A load with nothing painted is the case a revision cannot see.
// replace() installs a whole new canvas, which starts at revision 0.
// A session that has painted nothing was at revision 0 already.
// So the key has to hold the loads as well.
// Otherwise the frame after a load shows the sheet that is gone.
TEST(RenderSinkTest, Handle_UploadsAgainWhenAnUneditedSheetIsReplaced)
{
    NiceMock<MockWindow> window;
    NiceMock<MockRenderer> renderer;
    FakeSleeper sleeper;
    const EditorScene scene;
    EditorState state = opened();
    const UiOverlay overlay;

    ON_CALL(window, isOpen()).WillByDefault(Return(true));
    ON_CALL(window, renderer()).WillByDefault(ReturnRef(renderer));
    ON_CALL(renderer, createTexture(_))
        .WillByDefault(
            [](const Bitmap &)
            { return std::make_unique<NiceMock<MockTexture>>(); });

    EXPECT_CALL(renderer, createTexture(_)).Times(2);

    const antwika::console::ConsolePicture noConsole;
    RenderSink sink(
        window, scene, state, overlay, noConsole, sleeper,
        kFramePeriod);

    sink.handle(tickAt(1));

    ASSERT_EQ(state.image().revision(), 0U)
        << "the case is only the case while nothing has been painted";

    state.replace(Canvas::blank(Size{.width = 4, .height = 4}));
    sink.handle(tickAt(2));

    // And the sheet in hand is now the one that was loaded.
    EXPECT_EQ(state.image().size(), (Size{.width = 4, .height = 4}));
}

TEST(RenderSinkTest, Handle_DrawsNothingOnceTheWindowHasGone)
{
    NiceMock<MockWindow> window;
    NiceMock<MockRenderer> renderer;
    FakeSleeper sleeper;
    const EditorScene scene;
    const EditorState state = opened();
    const UiOverlay overlay;

    ON_CALL(window, isOpen()).WillByDefault(Return(false));

    EXPECT_CALL(window, renderer()).Times(0);

    const antwika::console::ConsolePicture noConsole;
    RenderSink sink(
        window, scene, state, overlay, noConsole, sleeper,
        kFramePeriod);

    sink.handle(tickAt(1));
}

TEST(RenderSinkTest, Handle_IgnoresAnythingThatIsNotATick)
{
    NiceMock<MockWindow> window;
    FakeSleeper sleeper;
    const EditorScene scene;
    const EditorState state = opened();
    const UiOverlay overlay;

    EXPECT_CALL(window, isOpen()).Times(0);

    const antwika::console::ConsolePicture noConsole;
    RenderSink sink(
        window, scene, state, overlay, noConsole, sleeper,
        kFramePeriod);

    sink.handle(TickEvent{
        .tick = 1, .event = Event{.name = "atlas_editor.nothing"}});
}
