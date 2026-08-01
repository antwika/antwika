#include <gtest/gtest.h>

#include <string>

#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/Size.hpp>

#include <antwika/i18n/MessageId.hpp>

#include "antwika/atlas_editor/Canvas.hpp"
#include "antwika/atlas_editor/CanvasView.hpp"
#include "antwika/atlas_editor/EditorState.hpp"
#include "antwika/atlas_editor/Palette.hpp"
#include "antwika/atlas_editor/Pixel.hpp"
#include "antwika/atlas_editor/TileGrid.hpp"
#include "antwika/atlas_editor/Tool.hpp"

using antwika::atlas_editor::Canvas;
using antwika::atlas_editor::defaultPalette;
using antwika::atlas_editor::EditorState;
using antwika::atlas_editor::Pixel;
using antwika::atlas_editor::scaleOf;
using antwika::atlas_editor::TileGrid;
using antwika::atlas_editor::Tool;
using antwika::atlas_editor::toolNameId;
using antwika::i18n::MessageId;
using antwika::gfx::Color;
using antwika::gfx::Point;
using antwika::gfx::Size;

namespace
{
    constexpr Size kCanvas{.width = 100, .height = 60};
    constexpr Size kSheet{.width = 10, .height = 10};

    // A ten by ten sheet centred on a hundred by sixty canvas.
    // One screen pixel per image pixel puts its corner here.
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
} // namespace

TEST(EditorStateTest, Construct_OpensOnTheWholeSheetWithPaintSelected)
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

    // No swatch holds that colour now, so none is shown as chosen.
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
        {.id = MessageId::AtlasSaved, .detail = "out.png"});

    ASSERT_TRUE(state.status().has_value());
    EXPECT_EQ(state.status()->id, MessageId::AtlasSaved);
    EXPECT_EQ(state.status()->detail, "out.png");
}

TEST(EditorStateTest, ToolNameId_NamesEveryToolAndSurvivesOneItLacks)
{
    EXPECT_EQ(toolNameId(Tool::Paint), MessageId::AtlasToolPaint);
    EXPECT_EQ(toolNameId(Tool::Erase), MessageId::AtlasToolErase);
    EXPECT_EQ(toolNameId(Tool::Pick), MessageId::AtlasToolPick);
    EXPECT_EQ(
        toolNameId(static_cast<Tool>(99)), MessageId::AtlasToolPaint);
}
