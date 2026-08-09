#include <gtest/gtest.h>

#include <cstddef>
#include <string>

#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/Size.hpp>

#include "antwika/atlas_editor/AtlasForm.hpp"
#include "antwika/atlas_editor/Canvas.hpp"
#include "antwika/atlas_editor/CanvasView.hpp"
#include "antwika/atlas_editor/EditorState.hpp"
#include "antwika/atlas_editor/MessageId.hpp"
#include "antwika/atlas_editor/Palette.hpp"
#include "antwika/atlas_editor/Pixel.hpp"
#include "antwika/atlas_editor/TileGrid.hpp"
#include "antwika/atlas_editor/Tool.hpp"

using antwika::atlas_editor::Canvas;
using antwika::atlas_editor::defaultPalette;
using antwika::atlas_editor::drawsShape;
using antwika::atlas_editor::EditorState;
using antwika::atlas_editor::MessageId;
using antwika::atlas_editor::Pixel;
using antwika::atlas_editor::scaleOf;
using antwika::atlas_editor::TileGrid;
using antwika::atlas_editor::Tool;
using antwika::atlas_editor::toolMark;
using antwika::atlas_editor::toolNameId;
using antwika::gfx::Color;
using antwika::gfx::Point;
using antwika::gfx::Size;

namespace
{
    constexpr Size kCanvas{.width = 100, .height = 60};
    constexpr Size kSheet{.width = 10, .height = 10};

    constexpr Point kCorner{.x = 45, .y = 25};

    constexpr Color kClear{
        .red = 0, .green = 0, .blue = 0, .alpha = 0};

    EditorState opened()
    {
        return EditorState{Canvas::blank(kSheet), TileGrid{}, kCanvas};
    }

    Point at(const int x, const int y)
    {
        return Point{.x = kCorner.x + x, .y = kCorner.y + y};
    }
}

TEST(EditorStateTest, Ctor_OpensOnTheWholeSheetWithPaintSelected)
{
    const EditorState state = opened();

    EXPECT_EQ(state.tool(), Tool::Paint);
    EXPECT_EQ(state.colorIndex(), 0U);
    EXPECT_EQ(state.color(), defaultPalette().front());
    EXPECT_TRUE(state.gridVisible());
    EXPECT_FALSE(state.hovered().has_value());
    EXPECT_FALSE(state.status().has_value());
    EXPECT_EQ(state.edits(), 0U);
    EXPECT_EQ(state.ticks(), 0U);
    EXPECT_EQ(state.saves(), 0U);
    EXPECT_EQ(state.loads(), 0U);
    EXPECT_FALSE(state.unsaved());
    EXPECT_EQ(state.canvas(), kCanvas);
    EXPECT_EQ(state.tiles(), TileGrid{});
    EXPECT_EQ(state.view().pan, kCorner);
}

TEST(EditorStateTest, ApplyAt_PaintsTheSelectedColourWhereThePointerIs)
{
    EditorState state = opened();
    state.selectColor(3);

    state.applyAt(at(2, 4));

    EXPECT_EQ(
        state.image().at(Pixel{.x = 2, .y = 4}), defaultPalette()[3]);
    EXPECT_EQ(state.edits(), 1U);
    EXPECT_TRUE(state.unsaved());
    EXPECT_EQ(state.hovered(), (Pixel{.x = 2, .y = 4}));
}

TEST(EditorStateTest, ApplyAt_CountsOneEditHoweverOftenAPixelIsCrossed)
{
    EditorState state = opened();

    state.applyAt(at(1, 1));
    state.applyAt(at(1, 1));

    EXPECT_EQ(state.edits(), 1U);
}

TEST(EditorStateTest, ApplyAt_ChangesNothingOutsideTheSheet)
{
    EditorState state = opened();

    state.applyAt(Point{.x = 0, .y = 0});

    EXPECT_EQ(state.edits(), 0U);
    EXPECT_FALSE(state.unsaved());
}

TEST(EditorStateTest, ApplyAt_ClearsThePixelWhenEraseIsSelected)
{
    EditorState state = opened();
    state.applyAt(at(5, 5));
    state.selectTool(Tool::Erase);

    state.applyAt(at(5, 5));

    EXPECT_EQ(state.image().at(Pixel{.x = 5, .y = 5}), kClear);
    EXPECT_EQ(state.edits(), 2U);
}

TEST(EditorStateTest, ApplyAt_TakesTheColourUnderThePointerWhenPicking)
{
    EditorState state = opened();
    state.selectColor(5);
    state.applyAt(at(7, 7));

    state.selectColor(0);
    state.selectTool(Tool::Pick);
    state.applyAt(at(7, 7));

    EXPECT_EQ(state.color(), defaultPalette()[5]);

    EXPECT_FALSE(state.colorIndex().has_value());
    EXPECT_EQ(state.edits(), 1U);
}

TEST(EditorStateTest, ApplyAt_PicksNothingFromOutsideTheSheet)
{
    EditorState state = opened();
    state.selectColor(2);
    state.selectTool(Tool::Pick);

    state.applyAt(Point{.x = 0, .y = 0});

    EXPECT_EQ(state.color(), defaultPalette()[2]);
    EXPECT_EQ(state.colorIndex(), 2U);
}

TEST(EditorStateTest, ApplyAt_FillsEveryPixelJoinedToTheOneClicked)
{
    EditorState state = opened();
    state.selectTool(Tool::Fill);

    state.applyAt(at(4, 4));

    EXPECT_EQ(state.edits(), kSheet.width * kSheet.height);
    EXPECT_EQ(
        state.image().at(Pixel{.x = 0, .y = 0}), defaultPalette()[0]);
    EXPECT_EQ(
        state.image().at(Pixel{.x = 9, .y = 9}), defaultPalette()[0]);
}

TEST(EditorStateTest, ApplyAt_FillStopsAtAPixelOfAnotherColour)
{
    EditorState state = opened();

    state.selectColor(1);
    for (int y = 0; y < 10; ++y)
    {
        state.applyAt(at(5, y));
    }

    state.selectColor(2);
    state.selectTool(Tool::Fill);
    state.applyAt(at(0, 0));

    EXPECT_EQ(
        state.image().at(Pixel{.x = 4, .y = 9}), defaultPalette()[2]);
    EXPECT_EQ(
        state.image().at(Pixel{.x = 5, .y = 9}), defaultPalette()[1]);
    EXPECT_EQ(
        state.image().at(Pixel{.x = 6, .y = 0}), kClear);
}

TEST(EditorStateTest, ApplyAt_FillsNothingOutsideTheSheet)
{
    EditorState state = opened();
    state.selectTool(Tool::Fill);

    state.applyAt(Point{.x = 0, .y = 0});

    EXPECT_EQ(state.edits(), 0U);
    EXPECT_FALSE(state.unsaved());
}

TEST(EditorStateTest, ApplyAt_FillsNothingWithTheColourAlreadyThere)
{
    EditorState state = opened();
    state.selectTool(Tool::Fill);
    state.applyAt(at(4, 4));

    state.applyAt(at(4, 4));

    EXPECT_EQ(state.edits(), kSheet.width * kSheet.height);
}

TEST(EditorStateTest, EraseAt_ClearsWhicheverToolIsSelected)
{
    EditorState state = opened();
    state.applyAt(at(3, 3));

    state.eraseAt(at(3, 3));

    EXPECT_EQ(state.tool(), Tool::Paint);
    EXPECT_EQ(state.image().at(Pixel{.x = 3, .y = 3}), kClear);
    EXPECT_EQ(state.edits(), 2U);
}

TEST(EditorStateTest, EraseAt_ChangesNothingWhereThereIsNothing)
{
    EditorState state = opened();

    state.eraseAt(at(3, 3));

    EXPECT_EQ(state.edits(), 0U);
}

TEST(EditorStateTest, SelectColor_IgnoresASwatchThePaletteDoesNotHave)
{
    EditorState state = opened();

    state.selectColor(defaultPalette().size());

    EXPECT_EQ(state.colorIndex(), 0U);
    EXPECT_EQ(state.color(), defaultPalette().front());
}

TEST(EditorStateTest, ToggleGrid_TurnsTheSlotBoundariesOffAndOnAgain)
{
    EditorState state = opened();

    state.toggleGrid();
    EXPECT_FALSE(state.gridVisible());

    state.toggleGrid();
    EXPECT_TRUE(state.gridVisible());
}

TEST(EditorStateTest, Select_MarksOutWhatTheDragCrossed)
{
    EditorState state = opened();
    state.selectTool(Tool::Select);

    state.beginSelecting(at(2, 2));
    state.dragSelectionTo(at(4, 5));
    state.finishSelecting(at(4, 5));

    ASSERT_TRUE(state.selection().has_value());
    EXPECT_EQ(state.selection()->origin, (Pixel{.x = 2, .y = 2}));
    EXPECT_EQ(state.selection()->size, (Size{.width = 3, .height = 4}));
    EXPECT_EQ(state.edits(), 0U);
}

TEST(EditorStateTest, Select_ShowsTheDragBeforeItIsFinished)
{
    EditorState state = opened();
    state.selectTool(Tool::Select);

    state.beginSelecting(at(2, 2));
    state.dragSelectionTo(at(4, 5));

    EXPECT_FALSE(state.selection().has_value());
    ASSERT_TRUE(state.shownSelection().has_value());
    EXPECT_EQ(
        state.shownSelection()->size, (Size{.width = 3, .height = 4}));
}

TEST(EditorStateTest, Select_MarksNothingForADragEntirelyOffTheSheet)
{
    EditorState state = opened();
    state.selectTool(Tool::Select);

    state.beginSelecting(Point{.x = 0, .y = 0});
    state.finishSelecting(Point{.x = 2, .y = 2});

    EXPECT_FALSE(state.selection().has_value());
}

TEST(EditorStateTest, Select_CutsAMarkedDragDownToTheSheet)
{
    EditorState state = opened();
    state.selectTool(Tool::Select);

    state.beginSelecting(at(8, 8));
    state.finishSelecting(at(40, 40));

    ASSERT_TRUE(state.selection().has_value());
    EXPECT_EQ(state.selection()->origin, (Pixel{.x = 8, .y = 8}));
    EXPECT_EQ(state.selection()->size, (Size{.width = 2, .height = 2}));
}

TEST(EditorStateTest, Select_DraggingFromInsideCarriesThePixels)
{
    EditorState state = opened();
    state.selectColor(4);
    state.applyAt(at(2, 2));

    state.selectTool(Tool::Select);
    state.beginSelecting(at(2, 2));
    state.finishSelecting(at(2, 2));

    state.beginSelecting(at(2, 2));
    state.dragSelectionTo(at(5, 3));
    state.finishSelecting(at(5, 3));

    EXPECT_EQ(state.image().at(Pixel{.x = 2, .y = 2}), kClear);
    EXPECT_EQ(
        state.image().at(Pixel{.x = 5, .y = 3}), defaultPalette()[4]);
    EXPECT_EQ(state.selection()->origin, (Pixel{.x = 5, .y = 3}));
}

TEST(EditorStateTest, Select_CarryingOntoItselfKeepsWhatItIsCarrying)
{
    EditorState state = opened();
    state.selectColor(4);
    state.applyAt(at(2, 2));
    state.applyAt(at(3, 2));

    state.selectTool(Tool::Select);
    state.beginSelecting(at(2, 2));
    state.finishSelecting(at(3, 2));

    state.beginSelecting(at(2, 2));
    state.finishSelecting(at(3, 2));

    EXPECT_EQ(state.image().at(Pixel{.x = 2, .y = 2}), kClear);
    EXPECT_EQ(
        state.image().at(Pixel{.x = 3, .y = 2}), defaultPalette()[4]);
    EXPECT_EQ(
        state.image().at(Pixel{.x = 4, .y = 2}), defaultPalette()[4]);
}

TEST(EditorStateTest, Select_CarryingRightOffTheSheetMovesNothing)
{
    EditorState state = opened();
    state.selectColor(4);
    state.applyAt(at(2, 2));

    state.selectTool(Tool::Select);
    state.beginSelecting(at(2, 2));
    state.finishSelecting(at(2, 2));

    state.beginSelecting(at(2, 2));
    state.finishSelecting(Point{.x = 0, .y = 0});

    EXPECT_FALSE(state.selection().has_value());
    EXPECT_EQ(
        state.image().at(Pixel{.x = 2, .y = 2}), defaultPalette()[4]);
}

TEST(EditorStateTest, Select_DraggingFromOutsideMarksANewRectangle)
{
    EditorState state = opened();
    state.selectTool(Tool::Select);

    state.beginSelecting(at(1, 1));
    state.finishSelecting(at(2, 2));

    state.beginSelecting(at(6, 6));
    state.finishSelecting(at(7, 7));

    EXPECT_EQ(state.selection()->origin, (Pixel{.x = 6, .y = 6}));
    EXPECT_EQ(state.edits(), 0U);
}

TEST(EditorStateTest, Select_ADragCarriedNowhereChangesNoPixel)
{
    EditorState state = opened();
    state.selectTool(Tool::Select);

    state.dragSelectionTo(at(3, 3));
    state.finishSelecting(at(3, 3));

    EXPECT_FALSE(state.selection().has_value());
    EXPECT_FALSE(state.shownSelection().has_value());
    EXPECT_EQ(state.edits(), 0U);
}

TEST(EditorStateTest, ClearSelection_DropsTheRectangleAndAnyDrag)
{
    EditorState state = opened();
    state.selectTool(Tool::Select);

    state.beginSelecting(at(1, 1));
    state.dragSelectionTo(at(4, 4));
    state.clearSelection();

    EXPECT_FALSE(state.selection().has_value());
    EXPECT_FALSE(state.shownSelection().has_value());
}

TEST(EditorStateTest, ApplyAt_PutsNoPixelDownWhileSelectIsInHand)
{
    EditorState state = opened();
    state.selectTool(Tool::Select);

    state.applyAt(at(3, 3));

    EXPECT_EQ(state.edits(), 0U);
    EXPECT_EQ(state.hovered(), (Pixel{.x = 3, .y = 3}));
}

TEST(EditorStateTest, Copy_TakesThePixelsAndPasteLandsThemAtThePointer)
{
    EditorState state = opened();
    state.selectColor(4);
    state.applyAt(at(1, 1));

    state.selectTool(Tool::Select);
    state.beginSelecting(at(1, 1));
    state.finishSelecting(at(1, 1));
    state.copySelection();

    EXPECT_TRUE(state.hasClipboard());

    state.moveTo(at(7, 5));
    state.pasteClipboard();

    EXPECT_EQ(
        state.image().at(Pixel{.x = 1, .y = 1}), defaultPalette()[4]);
    EXPECT_EQ(
        state.image().at(Pixel{.x = 7, .y = 5}), defaultPalette()[4]);
    EXPECT_EQ(state.selection()->origin, (Pixel{.x = 7, .y = 5}));
}

TEST(EditorStateTest, Cut_TakesThePixelsAndLeavesNothingBehind)
{
    EditorState state = opened();
    state.selectColor(4);
    state.applyAt(at(1, 1));

    state.selectTool(Tool::Select);
    state.beginSelecting(at(1, 1));
    state.finishSelecting(at(1, 1));
    state.cutSelection();

    EXPECT_TRUE(state.hasClipboard());
    EXPECT_EQ(state.image().at(Pixel{.x = 1, .y = 1}), kClear);

    state.moveTo(at(7, 5));
    state.pasteClipboard();

    EXPECT_EQ(
        state.image().at(Pixel{.x = 7, .y = 5}), defaultPalette()[4]);
}

TEST(EditorStateTest, Paste_WritesTransparencyOverWhatWasThere)
{
    EditorState state = opened();
    state.selectColor(4);
    state.applyAt(at(6, 6));

    state.selectTool(Tool::Select);
    state.beginSelecting(at(1, 1));
    state.finishSelecting(at(2, 2));
    state.copySelection();

    state.moveTo(at(6, 6));
    state.pasteClipboard();

    EXPECT_EQ(state.image().at(Pixel{.x = 6, .y = 6}), kClear);
}

TEST(EditorStateTest, Paste_LandsTheHalfOfItTheSheetHolds)
{
    EditorState state = opened();
    state.selectColor(4);
    state.applyAt(at(0, 0));
    state.applyAt(at(1, 0));

    state.selectTool(Tool::Select);
    state.beginSelecting(at(0, 0));
    state.finishSelecting(at(1, 0));
    state.copySelection();

    state.moveTo(at(9, 4));
    state.pasteClipboard();

    EXPECT_EQ(
        state.image().at(Pixel{.x = 9, .y = 4}), defaultPalette()[4]);
    EXPECT_EQ(state.selection()->size, (Size{.width = 1, .height = 1}));
}

TEST(EditorStateTest, CopyAndCut_DoNothingWithNothingMarked)
{
    EditorState state = opened();

    state.copySelection();
    state.cutSelection();

    EXPECT_FALSE(state.hasClipboard());
    EXPECT_EQ(state.edits(), 0U);
}

TEST(EditorStateTest, Paste_DoesNothingWithAnEmptyClipboard)
{
    EditorState state = opened();
    state.moveTo(at(3, 3));

    state.pasteClipboard();

    EXPECT_EQ(state.edits(), 0U);
    EXPECT_FALSE(state.selection().has_value());
}

TEST(EditorStateTest, Paste_DoesNothingWithNothingEverCopied)
{
    EditorState state = opened();

    state.pasteClipboard();

    EXPECT_EQ(state.edits(), 0U);
}

TEST(EditorStateTest, Paste_DoesNothingBeforeThePointerHasBeenAnywhere)
{
    EditorState state = opened();
    state.selectColor(4);
    state.applyAt(at(1, 1));

    state.selectTool(Tool::Select);
    state.beginSelecting(at(1, 1));
    state.finishSelecting(at(1, 1));
    state.copySelection();

    state.replace(Canvas::blank(kSheet));
    ASSERT_FALSE(state.hovered().has_value());

    const auto before = state.edits();
    state.pasteClipboard();

    EXPECT_EQ(state.edits(), before);
    EXPECT_FALSE(state.selection().has_value());
}

TEST(EditorStateTest, Cut_CountsOnlyThePixelsThatWereNotAlreadyClear)
{
    EditorState state = opened();
    state.selectColor(4);
    state.applyAt(at(1, 1));

    state.selectTool(Tool::Select);
    state.beginSelecting(at(1, 1));
    state.finishSelecting(at(2, 1));

    state.cutSelection();

    EXPECT_EQ(state.edits(), 2U);
}

TEST(EditorStateTest, Paste_MarksNothingWhenItLandsOffTheSheet)
{
    EditorState state = opened();
    state.selectColor(4);
    state.applyAt(at(1, 1));

    state.selectTool(Tool::Select);
    state.beginSelecting(at(1, 1));
    state.finishSelecting(at(1, 1));
    state.copySelection();

    state.moveTo(Point{.x = 0, .y = 0});
    state.pasteClipboard();

    EXPECT_FALSE(state.selection().has_value());
}

TEST(EditorStateTest, ToggleGuides_TurnsTheDiamondsOffAndOnAgain)
{
    EditorState state = opened();

    EXPECT_TRUE(state.guidesVisible());

    state.toggleGuides();
    EXPECT_FALSE(state.guidesVisible());

    state.toggleGuides();
    EXPECT_TRUE(state.guidesVisible());
}

TEST(EditorStateTest, Guides_AreTheDefaultSlotsWhicheverWayTheyAreShown)
{
    EditorState state = opened();
    const auto shown = state.guides();

    ASSERT_TRUE(shown.has_value());
    EXPECT_EQ(shown->pivot, (Point{.x = 32, .y = 64}));

    state.toggleGuides();
    EXPECT_EQ(state.guides(), shown);
}

TEST(EditorStateTest, Guides_AreNoneForASlotSizeWithNoDiamondInIt)
{
    const EditorState state{
        Canvas::blank(kSheet),
        TileGrid{.width = 8, .height = 8},
        kCanvas};

    EXPECT_TRUE(state.guidesVisible());
    EXPECT_FALSE(state.guides().has_value());
}

TEST(EditorStateTest, Zoom_ChangesHowLargeAPixelIsDrawn)
{
    EditorState state = opened();
    const Point middle{.x = 50, .y = 30};

    state.zoomIn(middle);
    EXPECT_EQ(scaleOf(state.view()), 2U);

    state.zoomOut(middle);
    EXPECT_EQ(scaleOf(state.view()), 1U);
}

TEST(EditorStateTest, PanBy_MovesTheSheetAndResetViewPutsItBack)
{
    EditorState state = opened();

    state.panBy(Point{.x = 12, .y = -4});
    EXPECT_NE(state.view().pan, kCorner);

    state.zoomIn(Point{.x = 0, .y = 0});
    state.resetView();

    EXPECT_EQ(state.view().pan, kCorner);
    EXPECT_EQ(scaleOf(state.view()), 1U);
}

TEST(EditorStateTest, MoveTo_ReportsThePixelUnderThePointer)
{
    EditorState state = opened();

    state.moveTo(at(9, 0));

    EXPECT_EQ(state.hovered(), (Pixel{.x = 9, .y = 0}));
}

TEST(EditorStateTest, MarkSaved_LeavesNothingToSaveUntilTheNextEdit)
{
    EditorState state = opened();
    state.applyAt(at(0, 0));

    state.markSaved();

    EXPECT_FALSE(state.unsaved());
    EXPECT_EQ(state.saves(), 1U);

    state.applyAt(at(1, 0));
    EXPECT_TRUE(state.unsaved());
}

TEST(EditorStateTest, Replace_TakesTheNewSheetAndRecentresOnIt)
{
    EditorState state = opened();
    state.applyAt(at(0, 0));
    state.moveTo(at(1, 1));

    state.replace(Canvas::blank(Size{.width = 20, .height = 20}));

    EXPECT_EQ(state.image().size(), (Size{.width = 20, .height = 20}));
    EXPECT_EQ(state.loads(), 1U);
    EXPECT_FALSE(state.unsaved());
    EXPECT_FALSE(state.hovered().has_value());
    EXPECT_EQ(state.view().pan, (Point{.x = 40, .y = 20}));
}

TEST(EditorStateTest, CloseStroke_RemembersTheSheetTheStrokeStartedOn)
{
    EditorState state = opened();
    state.selectColor(4);

    state.openStroke();
    state.applyAt(at(2, 2));
    state.applyAt(at(3, 2));
    state.closeStroke();

    EXPECT_EQ(state.undoDepth(), 1U);

    state.undo();

    EXPECT_EQ(state.image().at(Pixel{.x = 2, .y = 2}), kClear);
    EXPECT_EQ(state.image().at(Pixel{.x = 3, .y = 2}), kClear);
}

TEST(EditorStateTest, CloseStroke_RemembersNothingForAStrokeThatDidNothing)
{
    EditorState state = opened();

    state.openStroke();
    state.closeStroke();

    EXPECT_EQ(state.undoDepth(), 0U);
}

TEST(EditorStateTest, CloseStroke_RemembersNothingWithNoStrokeOpen)
{
    EditorState state = opened();
    state.selectColor(4);
    state.applyAt(at(2, 2));

    state.closeStroke();

    EXPECT_EQ(state.undoDepth(), 0U);
}

TEST(EditorStateTest, Undo_LeavesTheSheetAloneWithNothingToUndo)
{
    EditorState state = opened();
    state.selectColor(4);

    state.openStroke();
    state.applyAt(at(2, 2));
    state.closeStroke();

    state.undo();
    state.undo();

    EXPECT_EQ(state.image().at(Pixel{.x = 2, .y = 2}), kClear);
    EXPECT_EQ(state.undoDepth(), 0U);
}

TEST(EditorStateTest, Redo_LeavesTheSheetAloneWithNothingToRedo)
{
    EditorState state = opened();
    state.selectColor(4);

    state.openStroke();
    state.applyAt(at(2, 2));
    state.closeStroke();

    state.redo();

    EXPECT_EQ(
        state.image().at(Pixel{.x = 2, .y = 2}), defaultPalette()[4]);
    EXPECT_EQ(state.redoDepth(), 0U);
}

TEST(EditorStateTest, Undo_MovesTheRevisionOnSoTheSheetIsRedrawn)
{
    EditorState state = opened();
    state.selectColor(4);

    state.openStroke();
    state.applyAt(at(2, 2));
    state.closeStroke();

    const auto painted = state.image().revision();
    state.undo();

    EXPECT_GT(state.image().revision(), painted);
}

TEST(EditorStateTest, OpenStroke_DropsTheRedoRecordOnceANewStrokeLands)
{
    EditorState state = opened();
    state.selectColor(4);

    state.openStroke();
    state.applyAt(at(2, 2));
    state.closeStroke();

    state.undo();
    ASSERT_EQ(state.redoDepth(), 1U);

    state.openStroke();
    state.applyAt(at(5, 5));
    state.closeStroke();

    EXPECT_EQ(state.redoDepth(), 0U);
}

TEST(EditorStateTest, Replace_ForgetsTheStrokesTheOldSheetHad)
{
    EditorState state = opened();
    state.selectColor(4);

    state.openStroke();
    state.applyAt(at(2, 2));
    state.closeStroke();

    state.replace(Canvas::blank(Size{.width = 4, .height = 4}));

    EXPECT_EQ(state.undoDepth(), 0U);
}

TEST(EditorStateTest, CloseStroke_ForgetsTheOldestStrokePastTheLimit)
{
    constexpr std::size_t kDepth = 64;

    EditorState state = opened();
    state.selectColor(4);

    for (std::size_t stroke = 0; stroke <= kDepth; ++stroke)
    {
        state.openStroke();
        state.applyAt(at(
            static_cast<int>(stroke % 9),
            static_cast<int>(stroke / 9)));
        state.closeStroke();
    }

    EXPECT_EQ(state.undoDepth(), kDepth);
}

TEST(EditorStateTest, ShowMenu_HoldsTheMenuTheBarIsShowing)
{
    EditorState state = opened();

    EXPECT_EQ(state.openMenu(), antwika::atlas_editor::Menu::None);

    state.showMenu(antwika::atlas_editor::Menu::View);

    EXPECT_EQ(state.openMenu(), antwika::atlas_editor::Menu::View);
}

TEST(EditorStateTest, NoteTick_CountsWhatTheLoopHasStepped)
{
    EditorState state = opened();

    state.noteTick();
    state.noteTick();

    EXPECT_EQ(state.ticks(), 2U);
}

TEST(EditorStateTest, SetStatus_KeepsTheLastThingWorthSaying)
{
    EditorState state = opened();

    state.setStatus(
        {.id = MessageId::Saved, .detail = "out.png"});

    ASSERT_TRUE(state.status().has_value());
    EXPECT_EQ(state.status()->id, MessageId::Saved);
    EXPECT_EQ(state.status()->detail, "out.png");
}

TEST(EditorStateTest, ToolNameId_NamesEveryToolAndSurvivesOneItLacks)
{
    EXPECT_EQ(toolNameId(Tool::Paint), MessageId::ToolPaint);
    EXPECT_EQ(toolNameId(Tool::Erase), MessageId::ToolErase);
    EXPECT_EQ(toolNameId(Tool::Pick), MessageId::ToolPick);
    EXPECT_EQ(toolNameId(Tool::Line), MessageId::ToolLine);
    EXPECT_EQ(toolNameId(Tool::Ellipse), MessageId::ToolEllipse);
    EXPECT_EQ(
        toolNameId(static_cast<Tool>(99)), MessageId::ToolPaint);
}

TEST(EditorStateTest, ToolMark_MarksEveryToolAndSurvivesOneItLacks)
{
    EXPECT_EQ(toolMark(Tool::Line), "Li");
    EXPECT_EQ(toolMark(Tool::Ellipse), "El");
    EXPECT_EQ(toolMark(static_cast<Tool>(99)), toolMark(Tool::Paint));
}

TEST(EditorStateTest, DrawsShape_HoldsForTheTwoToolsThatDragAShape)
{
    EXPECT_TRUE(drawsShape(Tool::Line));
    EXPECT_TRUE(drawsShape(Tool::Ellipse));
    EXPECT_FALSE(drawsShape(Tool::Paint));
    EXPECT_FALSE(drawsShape(Tool::Select));
}

TEST(EditorStateTest, FinishStroke_PaintsTheLineTheDragCrossed)
{
    EditorState state = opened();
    state.selectTool(Tool::Line);
    state.selectColor(4);

    state.beginStroke(at(2, 2));
    state.dragStrokeTo(at(5, 2));
    state.finishStroke(at(5, 2));

    EXPECT_EQ(
        state.image().at(Pixel{.x = 2, .y = 2}), defaultPalette()[4]);
    EXPECT_EQ(
        state.image().at(Pixel{.x = 4, .y = 2}), defaultPalette()[4]);
    EXPECT_EQ(
        state.image().at(Pixel{.x = 5, .y = 2}), defaultPalette()[4]);
    EXPECT_EQ(state.image().at(Pixel{.x = 6, .y = 2}), kClear);
    EXPECT_EQ(state.edits(), 4U);
}

TEST(EditorStateTest, FinishStroke_PaintsTheEllipseTheDragBoxedIn)
{
    EditorState state = opened();
    state.selectTool(Tool::Ellipse);
    state.selectColor(4);

    state.beginStroke(at(1, 1));
    state.finishStroke(at(7, 7));

    EXPECT_EQ(
        state.image().at(Pixel{.x = 4, .y = 1}), defaultPalette()[4]);
    EXPECT_EQ(state.image().at(Pixel{.x = 4, .y = 4}), kClear);
}

TEST(EditorStateTest, FinishStroke_CountsNoEditForAPixelAlreadyPainted)
{
    EditorState state = opened();
    state.selectTool(Tool::Line);
    state.selectColor(4);

    state.beginStroke(at(2, 2));
    state.finishStroke(at(5, 2));

    const auto drawn = state.edits();

    state.beginStroke(at(2, 2));
    state.finishStroke(at(5, 2));

    EXPECT_EQ(state.edits(), drawn);
}

TEST(EditorStateTest, FinishStroke_PaintsNothingWithNoStrokeBegun)
{
    EditorState state = opened();
    state.selectTool(Tool::Line);

    state.finishStroke(at(5, 2));

    EXPECT_EQ(state.edits(), 0U);
}

TEST(EditorStateTest, ShownStroke_ShowsADragUntilItIsLetGo)
{
    EditorState state = opened();
    state.selectTool(Tool::Line);

    state.beginStroke(at(2, 2));
    state.dragStrokeTo(at(5, 2));

    ASSERT_TRUE(state.shownStroke().has_value());
    EXPECT_EQ(state.shownStroke()->from, (Pixel{.x = 2, .y = 2}));
    EXPECT_EQ(state.shownStroke()->to, (Pixel{.x = 5, .y = 2}));

    state.finishStroke(at(5, 2));

    EXPECT_FALSE(state.shownStroke().has_value());
}

TEST(EditorStateTest, ShownStroke_ShowsNothingForAToolThatDrawsNoShape)
{
    EditorState state = opened();
    state.selectTool(Tool::Select);

    state.beginSelecting(at(2, 2));
    state.dragSelectionTo(at(5, 2));

    EXPECT_FALSE(state.shownStroke().has_value());
}

TEST(EditorStateTest, ShownSelection_IgnoresADragThatIsDrawingAShape)
{
    EditorState state = opened();
    state.selectTool(Tool::Line);

    state.beginStroke(at(2, 2));
    state.dragStrokeTo(at(5, 5));

    EXPECT_FALSE(state.shownSelection().has_value());
}

TEST(EditorStateTest, DragStrokeTo_MovesNothingWithNoStrokeBegun)
{
    EditorState state = opened();
    state.selectTool(Tool::Line);

    state.dragStrokeTo(at(5, 2));

    EXPECT_FALSE(state.shownStroke().has_value());
}

TEST(EditorStateTest, FocusField_IgnoresAFieldTheFormDoesNotCarry)
{
    EditorState state = opened();
    state.showNewAtlas();
    state.focusField(1);

    state.focusField(antwika::atlas_editor::kAtlasFieldCount);

    EXPECT_EQ(state.formField(), 1U);
}

TEST(EditorStateTest, EraseSelection_EmptiesTheMarkedPixelsOnly)
{
    EditorState state = opened();
    state.selectColor(4);

    state.openStroke();
    state.applyAt(at(2, 2));
    state.applyAt(at(6, 2));
    state.closeStroke();

    state.beginSelecting(at(1, 1));
    state.finishSelecting(at(3, 3));

    state.eraseSelection();

    EXPECT_EQ(state.image().at(Pixel{.x = 2, .y = 2}), kClear);
    EXPECT_EQ(
        state.image().at(Pixel{.x = 6, .y = 2}), defaultPalette()[4]);
}

TEST(EditorStateTest, EraseSelection_LeavesTheClipboardAsItWas)
{
    EditorState state = opened();
    state.selectColor(4);

    state.openStroke();
    state.applyAt(at(2, 2));
    state.closeStroke();

    state.beginSelecting(at(1, 1));
    state.finishSelecting(at(3, 3));

    state.eraseSelection();

    EXPECT_FALSE(state.hasClipboard());
}

TEST(EditorStateTest, EraseSelection_EmptiesNothingWithNoSelection)
{
    EditorState state = opened();
    state.selectColor(4);

    state.openStroke();
    state.applyAt(at(2, 2));
    state.closeStroke();

    state.eraseSelection();

    EXPECT_EQ(
        state.image().at(Pixel{.x = 2, .y = 2}), defaultPalette()[4]);
}

TEST(EditorStateTest, TogglePivot_ShowsThePivotWithoutTheGuides)
{
    EditorState state = opened();

    EXPECT_FALSE(state.pivotVisible());

    state.togglePivot();

    EXPECT_TRUE(state.pivotVisible());
    EXPECT_TRUE(state.guidesVisible());
}

TEST(EditorStateTest, TogglePointerBorder_StartsOnAndTurnsOff)
{
    EditorState state = opened();

    EXPECT_TRUE(state.pointerBorderVisible());

    state.togglePointerBorder();

    EXPECT_FALSE(state.pointerBorderVisible());
}

TEST(EditorStateTest, TakePreset_FillsTheFormFromTheShippedShape)
{
    EditorState state = opened();
    state.showNewAtlas();

    state.takePreset(2);

    EXPECT_EQ(state.form(), antwika::atlas_editor::presetForm(2));
}
