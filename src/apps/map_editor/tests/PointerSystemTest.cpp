#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

#include <antwika/ecs/World.hpp>
#include <antwika/geometry/Grid.hpp>
#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/gfx/ViewportRenderer.hpp>
#include <antwika/gfx/mocks/MockRenderer.hpp>
#include <antwika/input/InputEvent.hpp>
#include <antwika/input/Key.hpp>
#include <antwika/input/MouseButton.hpp>
#include <antwika/log/mocks/MockLogger.hpp>
#include <antwika/tilemap/MapHeader.hpp>
#include <antwika/tilemap/TileMap.hpp>
#include <antwika/tileset/Tileset.hpp>
#include <antwika/time/Tick.hpp>

#include "antwika/map_editor/EditorStore.hpp"
#include "antwika/map_editor/PointerSystem.hpp"

using antwika::ecs::World;
using antwika::geometry::GridCell;
using antwika::gfx::Point;
using antwika::gfx::Size;
using antwika::gfx::ViewportRenderer;
using antwika::gfx::mocks::MockRenderer;
using antwika::input::InputEvent;
using antwika::input::Key;
using antwika::input::KeyPressed;
using antwika::input::MouseButton;
using antwika::input::PointerButtonPressed;
using antwika::input::PointerButtonReleased;
using antwika::input::PointerMoved;
using antwika::input::PointerScrolled;
using antwika::log::mocks::MockLogger;
using antwika::map_editor::DialogMode;
using antwika::map_editor::EditorStore;
using antwika::map_editor::EditorView;
using antwika::map_editor::GestureKind;
using antwika::map_editor::PointerSystem;
using antwika::map_editor::SignedCell;
using antwika::map_editor::TilesetDoc;
using antwika::tilemap::MapHeader;
using antwika::tilemap::TileMap;
using antwika::tileset::addSprite;
using ::testing::NiceMock;

namespace
{
    constexpr antwika::time::Tick kTick{};

    constexpr Size kCanvas{.width = 320, .height = 270};

    constexpr std::uint32_t kMapCells = 4;

    constexpr std::size_t kFullLibraryPage = 66;

    [[nodiscard]] GridCell cellAt(
        const std::uint32_t column, const std::uint32_t row)
    {
        return GridCell{.column = column, .row = row};
    }

    [[nodiscard]] Point pointAt(
        const std::int32_t x, const std::int32_t y)
    {
        return Point{.x = x, .y = y};
    }

    [[nodiscard]] InputEvent movedTo(
        const std::int32_t x, const std::int32_t y)
    {
        return PointerMoved{.position = {.x = x, .y = y}};
    }

    [[nodiscard]] InputEvent pressedAt(
        const MouseButton button,
        const std::int32_t x,
        const std::int32_t y,
        const bool control = false)
    {
        return PointerButtonPressed{
            .button = button,
            .position = {.x = x, .y = y},
            .modifiers = {.control = control}};
    }

    [[nodiscard]] InputEvent releasedOf(const MouseButton button)
    {
        return PointerButtonReleased{.button = button};
    }

    [[nodiscard]] InputEvent scrolledBy(
        const std::int32_t horizontal, const std::int32_t vertical)
    {
        return PointerScrolled{
            .horizontal = horizontal, .vertical = vertical};
    }

    class PointerSystemTest : public ::testing::Test
    {
    protected:
        void run()
        {
            PointerSystem system{store, view};

            system.update(world, kTick);
        }

        void feed(std::vector<InputEvent> events)
        {
            store.input.events = std::move(events);
            run();
        }

        void openTileset(const std::size_t sprites)
        {
            TilesetDoc doc;

            for (std::size_t at = 0; at < sprites; ++at)
            {
                static_cast<void>(addSprite(doc.data, 0));
            }

            store.tilesets.open.push_back(std::move(doc));
        }

        NiceMock<MockRenderer> inner;
        NiceMock<MockLogger> logger;
        ViewportRenderer view{inner, kCanvas, kCanvas};
        World world{logger};
        EditorStore store{
            .state = {.map = TileMap{MapHeader{}, kMapCells, kMapCells}}};
    };
}

TEST_F(PointerSystemTest, Update_DropsTheGesturesOfTheLastTick)
{
    store.input.gestures.push_back({});
    store.input.sheetGestures.push_back({});
    store.input.pressed = true;

    run();

    EXPECT_TRUE(store.input.gestures.empty());
    EXPECT_TRUE(store.input.sheetGestures.empty());
    EXPECT_FALSE(store.input.pressed);
}

TEST_F(PointerSystemTest, Update_IgnoresAKeyEvent)
{
    feed({KeyPressed{.key = Key::A}});

    EXPECT_FALSE(store.input.canvasPointer.has_value());
    EXPECT_TRUE(store.input.gestures.empty());
}

TEST_F(PointerSystemTest, Update_RemembersWhereThePointerMovedTo)
{
    feed({movedTo(100, 100)});

    ASSERT_TRUE(store.input.canvasPointer.has_value());
    EXPECT_EQ(*store.input.canvasPointer, pointAt(100, 100));
}

TEST_F(PointerSystemTest, Update_HoversTheCellUnderAMove)
{
    feed({movedTo(20, 30)});

    EXPECT_EQ(store.state.hovered, cellAt(1, 1));
}

TEST_F(PointerSystemTest, Update_ReportsAMoveGestureOverTheMap)
{
    feed({movedTo(20, 30)});

    ASSERT_EQ(store.input.gestures.size(), 1U);

    const auto &gesture = store.input.gestures.front();

    EXPECT_EQ(gesture.kind, GestureKind::Move);
    EXPECT_EQ(gesture.cell, cellAt(1, 1));
    EXPECT_EQ(gesture.signedCell.column, 1);
    EXPECT_EQ(gesture.signedCell.row, 1);
    EXPECT_FALSE(gesture.erase);
}

TEST_F(PointerSystemTest, Update_MarksAMoveGestureErasingWhileTheRightHolds)
{
    store.input.erasing = true;

    feed({movedTo(20, 30)});

    ASSERT_EQ(store.input.gestures.size(), 1U);
    EXPECT_TRUE(store.input.gestures.front().erase);
}

TEST_F(PointerSystemTest, Update_HoversLeftOfTheMapOnlyWithinTheMargin)
{
    store.camera.panX = 20.0F;

    feed({movedTo(0, 10)});

    ASSERT_TRUE(store.state.hoveredBeyond.has_value());
    EXPECT_EQ(store.state.hoveredBeyond->column, -2);
    EXPECT_EQ(store.state.hoveredBeyond->row, 0);

    store.camera.panX = 100.0F;

    feed({movedTo(0, 10)});

    EXPECT_FALSE(store.state.hoveredBeyond.has_value());
}

TEST_F(PointerSystemTest, Update_HoversAboveTheMapOnlyWithinTheMargin)
{
    store.camera.panY = 20.0F;

    feed({movedTo(20, 10)});

    ASSERT_TRUE(store.state.hoveredBeyond.has_value());
    EXPECT_EQ(store.state.hoveredBeyond->column, 1);
    EXPECT_EQ(store.state.hoveredBeyond->row, -2);

    store.camera.panY = 100.0F;

    feed({movedTo(20, 10)});

    EXPECT_FALSE(store.state.hoveredBeyond.has_value());
}

TEST_F(PointerSystemTest, Update_HoversRightOfTheMapOnlyWithinTheMargin)
{
    feed({movedTo(80, 10)});

    ASSERT_TRUE(store.state.hoveredBeyond.has_value());
    EXPECT_EQ(store.state.hoveredBeyond->column, 5);
    EXPECT_EQ(store.state.hoveredBeyond->row, 0);

    feed({movedTo(112, 10)});

    EXPECT_FALSE(store.state.hoveredBeyond.has_value());
}

TEST_F(PointerSystemTest, Update_HoversBelowTheMapOnlyWithinTheMargin)
{
    feed({movedTo(0, 90)});

    ASSERT_TRUE(store.state.hoveredBeyond.has_value());
    EXPECT_EQ(store.state.hoveredBeyond->column, 0);
    EXPECT_EQ(store.state.hoveredBeyond->row, 5);

    feed({movedTo(0, 122)});

    EXPECT_FALSE(store.state.hoveredBeyond.has_value());
}

TEST_F(PointerSystemTest, Update_HoldsNoBeyondHoverInsideTheMap)
{
    store.state.hoveredBeyond = SignedCell{.column = 9, .row = 9};

    feed({movedTo(20, 30)});

    EXPECT_FALSE(store.state.hoveredBeyond.has_value());
}

TEST_F(PointerSystemTest, Update_ForgetsTheBeyondHoverOutsideTheMapView)
{
    for (const auto position :
         {pointAt(-1, 100), pointAt(320, 100), pointAt(100, 9)})
    {
        SCOPED_TRACE(position.x);
        store.state.hoveredBeyond = SignedCell{.column = 1, .row = 1};

        feed({movedTo(position.x, position.y)});

        EXPECT_FALSE(store.state.hoveredBeyond.has_value());
        EXPECT_TRUE(store.input.gestures.empty());
    }
}

TEST_F(PointerSystemTest, Update_PansTheCameraWhileTheMiddleButtonDrags)
{
    feed({pressedAt(MouseButton::Middle, 100, 100), movedTo(110, 120)});

    EXPECT_TRUE(store.input.panning);
    EXPECT_FLOAT_EQ(store.camera.panX, 10.0F);
    EXPECT_FLOAT_EQ(store.camera.panY, 20.0F);
}

TEST_F(PointerSystemTest, Update_ClampsThePanToKeepTheMapInView)
{
    feed({pressedAt(MouseButton::Middle, 0, 100), movedTo(300, 100)});

    EXPECT_FLOAT_EQ(store.camera.panX, 272.0F);
}

TEST_F(PointerSystemTest, Update_StopsPanningOnTheMiddleRelease)
{
    feed({pressedAt(MouseButton::Middle, 100, 100),
          releasedOf(MouseButton::Middle),
          movedTo(110, 120)});

    EXPECT_FALSE(store.input.panning);
    EXPECT_FLOAT_EQ(store.camera.panX, 0.0F);
}

TEST_F(PointerSystemTest, Update_StartsNoPanOutsideTheMapView)
{
    feed({pressedAt(MouseButton::Middle, 100, 2)});

    EXPECT_FALSE(store.input.panning);
}

TEST_F(PointerSystemTest, Update_StartsNoPanWhileAModalIsOpen)
{
    store.palette.open = true;

    feed({pressedAt(MouseButton::Middle, 100, 100)});

    EXPECT_FALSE(store.input.panning);
}

TEST_F(PointerSystemTest, Update_StartsNoPanInTheTilesView)
{
    store.view = EditorView::Tiles;

    feed({pressedAt(MouseButton::Middle, 100, 100)});

    EXPECT_FALSE(store.input.panning);
}

TEST_F(PointerSystemTest, Update_MarksThePointerDownOnALeftPress)
{
    feed({pressedAt(MouseButton::Left, 20, 30)});

    EXPECT_TRUE(store.input.down);
    EXPECT_TRUE(store.input.pressed);
}

TEST_F(PointerSystemTest, Update_ReportsAPressGestureOnTheMap)
{
    feed({pressedAt(MouseButton::Left, 20, 30)});

    ASSERT_EQ(store.input.gestures.size(), 1U);

    const auto &gesture = store.input.gestures.front();

    EXPECT_EQ(gesture.kind, GestureKind::Press);
    EXPECT_EQ(gesture.cell, cellAt(1, 1));
    EXPECT_EQ(gesture.signedCell.column, 1);
    EXPECT_FALSE(gesture.erase);
}

TEST_F(PointerSystemTest, Update_HoversTheCellOfALeftPress)
{
    feed({pressedAt(MouseButton::Left, 20, 30)});

    EXPECT_EQ(store.state.hovered, cellAt(1, 1));
}

TEST_F(PointerSystemTest, Update_KeepsTheHoverOnAPressBeyondTheEdge)
{
    store.state.hovered = cellAt(2, 2);
    store.camera.panX = 20.0F;

    feed({pressedAt(MouseButton::Left, 0, 10)});

    EXPECT_EQ(store.state.hovered, cellAt(2, 2));
    ASSERT_EQ(store.input.gestures.size(), 1U);
    EXPECT_EQ(store.input.gestures.front().signedCell.column, -2);
}

TEST_F(PointerSystemTest, Update_PendsAPickWhileThePickerIsActive)
{
    store.picker.active = true;

    feed({pressedAt(MouseButton::Left, 20, 30)});

    ASSERT_TRUE(store.picker.pending.has_value());
    EXPECT_EQ(*store.picker.pending, pointAt(20, 20));
    EXPECT_TRUE(store.input.gestures.empty());
}

TEST_F(PointerSystemTest, Update_PendsAPickOnAControlHeldLeftPress)
{
    feed({pressedAt(MouseButton::Left, 20, 30, true)});

    ASSERT_TRUE(store.picker.pending.has_value());
    EXPECT_EQ(*store.picker.pending, pointAt(20, 20));
    EXPECT_TRUE(store.input.gestures.empty());
}

TEST_F(PointerSystemTest, Update_ReportsNoPressGestureOverTheUi)
{
    store.ui.pointerOverUi = true;

    feed({pressedAt(MouseButton::Left, 20, 30)});

    EXPECT_TRUE(store.input.gestures.empty());
    EXPECT_TRUE(store.input.pressed);
}

TEST_F(PointerSystemTest, Update_ReportsNoPressGestureUnderAnOpenMenu)
{
    store.ui.openMenu = 0;

    feed({pressedAt(MouseButton::Left, 20, 30)});

    EXPECT_TRUE(store.input.gestures.empty());
}

TEST_F(PointerSystemTest, Update_ReportsNoPressGestureWhileAModalIsOpen)
{
    store.dialog.mode = DialogMode::Open;

    feed({pressedAt(MouseButton::Left, 20, 30)});

    EXPECT_TRUE(store.input.gestures.empty());
}

TEST_F(PointerSystemTest, Update_ReportsNoPressGestureOutsideTheMapView)
{
    feed({pressedAt(MouseButton::Left, 100, 2)});

    EXPECT_TRUE(store.input.gestures.empty());
    EXPECT_TRUE(store.input.pressed);
}

TEST_F(PointerSystemTest, Update_IgnoresAPressUnderTheOpenConsole)
{
    store.input.consoleVisible = true;
    store.input.consoleHeightCanvas = 60;

    feed({pressedAt(MouseButton::Left, 20, 30)});

    EXPECT_FALSE(store.input.down);
    EXPECT_TRUE(store.input.gestures.empty());
}

TEST_F(PointerSystemTest, Update_HandlesAPressBelowTheOpenConsole)
{
    store.input.consoleVisible = true;
    store.input.consoleHeightCanvas = 20;

    feed({pressedAt(MouseButton::Left, 20, 30)});

    EXPECT_TRUE(store.input.down);
    EXPECT_EQ(store.input.gestures.size(), 1U);
}

TEST_F(PointerSystemTest, Update_ErasesOnARightPressOverTheMap)
{
    feed({pressedAt(MouseButton::Right, 20, 30)});

    EXPECT_TRUE(store.input.erasing);
    ASSERT_EQ(store.input.gestures.size(), 1U);

    const auto &gesture = store.input.gestures.front();

    EXPECT_EQ(gesture.kind, GestureKind::Press);
    EXPECT_EQ(gesture.cell, cellAt(1, 1));
    EXPECT_EQ(gesture.signedCell.row, 1);
    EXPECT_TRUE(gesture.erase);
}

TEST_F(PointerSystemTest, Update_ErasesNothingOnARightPressOverTheUi)
{
    store.ui.pointerOverUi = true;

    feed({pressedAt(MouseButton::Right, 20, 30)});

    EXPECT_FALSE(store.input.erasing);
    EXPECT_TRUE(store.input.gestures.empty());
}

TEST_F(PointerSystemTest, Update_ErasesNothingOnARightPressUnderAMenu)
{
    store.ui.openMenu = 0;

    feed({pressedAt(MouseButton::Right, 20, 30)});

    EXPECT_FALSE(store.input.erasing);
}

TEST_F(PointerSystemTest, Update_ErasesNothingOnARightPressUnderAModal)
{
    store.rules.open = true;

    feed({pressedAt(MouseButton::Right, 20, 30)});

    EXPECT_FALSE(store.input.erasing);
}

TEST_F(PointerSystemTest, Update_ErasesNothingWhileThePickerIsActive)
{
    store.picker.active = true;

    feed({pressedAt(MouseButton::Right, 20, 30)});

    EXPECT_FALSE(store.input.erasing);
}

TEST_F(PointerSystemTest, Update_ErasesNothingOnARightPressOffTheMapView)
{
    feed({pressedAt(MouseButton::Right, 100, 2)});

    EXPECT_FALSE(store.input.erasing);
}

TEST_F(PointerSystemTest, Update_IgnoresAnExtraButtonPress)
{
    feed({pressedAt(MouseButton::X1, 20, 30)});

    EXPECT_FALSE(store.input.down);
    EXPECT_TRUE(store.input.gestures.empty());
}

TEST_F(PointerSystemTest, Update_ReportsAReleaseGestureOnALeftRelease)
{
    store.input.down = true;

    feed({releasedOf(MouseButton::Left)});

    EXPECT_FALSE(store.input.down);
    ASSERT_EQ(store.input.gestures.size(), 1U);
    EXPECT_EQ(
        store.input.gestures.front().kind, GestureKind::Release);
}

TEST_F(PointerSystemTest, Update_EndsTheEraseOnARightRelease)
{
    store.input.erasing = true;

    feed({releasedOf(MouseButton::Right)});

    EXPECT_FALSE(store.input.erasing);
    ASSERT_EQ(store.input.gestures.size(), 1U);
    EXPECT_EQ(
        store.input.gestures.front().kind, GestureKind::Release);
    EXPECT_TRUE(store.input.gestures.front().erase);
}

TEST_F(PointerSystemTest, Update_ReportsNoReleaseWhenNothingWasErasing)
{
    feed({releasedOf(MouseButton::Right)});

    EXPECT_TRUE(store.input.gestures.empty());
}

TEST_F(PointerSystemTest, Update_IgnoresAnExtraButtonRelease)
{
    store.input.down = true;

    feed({releasedOf(MouseButton::X1)});

    EXPECT_TRUE(store.input.down);
    EXPECT_TRUE(store.input.gestures.empty());
}

TEST_F(PointerSystemTest, Update_ZoomsInOnAScrollUpOverTheMap)
{
    store.input.canvasPointer = pointAt(20, 30);

    feed({scrolledBy(0, 1)});

    EXPECT_EQ(store.camera.step, 2U);
}

TEST_F(PointerSystemTest, Update_ZoomsOutOnAScrollDownOverTheMap)
{
    store.input.canvasPointer = pointAt(20, 30);

    feed({scrolledBy(0, -1)});

    EXPECT_EQ(store.camera.step, 0U);
}

TEST_F(PointerSystemTest, Update_PansSidewaysOnAHorizontalScroll)
{
    store.input.canvasPointer = pointAt(20, 30);

    feed({scrolledBy(1, 0)});

    EXPECT_FLOAT_EQ(store.camera.panX, -16.0F);
}

TEST_F(PointerSystemTest, Update_MovesNothingOnAScrollWithNoDelta)
{
    store.input.canvasPointer = pointAt(20, 30);

    feed({scrolledBy(0, 0)});

    EXPECT_EQ(store.camera.step, 1U);
    EXPECT_FLOAT_EQ(store.camera.panX, 0.0F);
}

TEST_F(PointerSystemTest, Update_IgnoresAScrollBeforeThePointerIsKnown)
{
    feed({scrolledBy(0, 1)});

    EXPECT_EQ(store.camera.step, 1U);
}

TEST_F(PointerSystemTest, Update_IgnoresAScrollWhileAModalIsOpen)
{
    store.keys.open = true;
    store.input.canvasPointer = pointAt(20, 30);

    feed({scrolledBy(0, 1)});

    EXPECT_EQ(store.camera.step, 1U);
}

TEST_F(PointerSystemTest, Update_IgnoresAScrollUnderTheOpenConsole)
{
    store.input.consoleVisible = true;
    store.input.consoleHeightCanvas = 60;
    store.input.canvasPointer = pointAt(20, 30);

    feed({scrolledBy(0, 1)});

    EXPECT_EQ(store.camera.step, 1U);
}

TEST_F(PointerSystemTest, Update_IgnoresAScrollOutsideTheMapView)
{
    store.input.canvasPointer = pointAt(100, 2);

    feed({scrolledBy(0, 1)});

    EXPECT_EQ(store.camera.step, 1U);
}

TEST_F(PointerSystemTest, Update_IgnoresAScrollInTheCharactersView)
{
    store.view = EditorView::Characters;
    store.input.canvasPointer = pointAt(20, 30);

    feed({scrolledBy(0, 1)});

    EXPECT_EQ(store.camera.step, 1U);
}

TEST_F(PointerSystemTest, Update_PagesTheLibraryForwardOnAScrollDown)
{
    store.view = EditorView::Tiles;
    openTileset(kFullLibraryPage);
    store.input.canvasPointer = pointAt(200, 100);

    feed({scrolledBy(0, -1)});

    EXPECT_EQ(store.tilesets.libraryPage, 1U);
}

TEST_F(PointerSystemTest, Update_PagesTheLibraryBackOnAScrollUp)
{
    store.view = EditorView::Tiles;
    openTileset(kFullLibraryPage);
    store.tilesets.libraryPage = 1;
    store.input.canvasPointer = pointAt(200, 100);

    feed({scrolledBy(0, 1)});

    EXPECT_EQ(store.tilesets.libraryPage, 0U);
}

TEST_F(PointerSystemTest, Update_PagesNothingOnASidewaysLibraryScroll)
{
    store.view = EditorView::Tiles;
    openTileset(kFullLibraryPage);
    store.tilesets.libraryPage = 1;
    store.input.canvasPointer = pointAt(200, 100);

    feed({scrolledBy(1, 0)});

    EXPECT_EQ(store.tilesets.libraryPage, 1U);
}

TEST_F(PointerSystemTest, Update_PagesNothingOnAScrollOffTheLibrary)
{
    store.view = EditorView::Tiles;
    openTileset(kFullLibraryPage);
    store.tilesets.libraryPage = 1;
    store.input.canvasPointer = pointAt(10, 100);

    feed({scrolledBy(0, -1)});

    EXPECT_EQ(store.tilesets.libraryPage, 1U);
}

TEST_F(PointerSystemTest, Update_ReportsASheetMoveInTheTilesView)
{
    store.view = EditorView::Tiles;

    feed({movedTo(100, 100)});

    ASSERT_EQ(store.input.sheetGestures.size(), 1U);

    const auto &gesture = store.input.sheetGestures.front();

    EXPECT_EQ(gesture.kind, GestureKind::Move);
    EXPECT_EQ(gesture.pixel, pointAt(100, 100));
    EXPECT_TRUE(store.input.gestures.empty());
}

TEST_F(PointerSystemTest, Update_ReportsNoSheetMoveOffTheTilesCanvas)
{
    store.view = EditorView::Tiles;

    for (const auto position :
         {pointAt(-1, 100), pointAt(320, 100), pointAt(100, 9)})
    {
        SCOPED_TRACE(position.x);

        feed({movedTo(position.x, position.y)});

        EXPECT_TRUE(store.input.sheetGestures.empty());
    }
}

TEST_F(PointerSystemTest, Update_ReportsASheetPressWithInkOnALeftPress)
{
    store.view = EditorView::Tiles;

    feed({pressedAt(MouseButton::Left, 100, 100)});

    ASSERT_EQ(store.input.sheetGestures.size(), 1U);

    const auto &gesture = store.input.sheetGestures.front();

    EXPECT_EQ(gesture.kind, GestureKind::Press);
    EXPECT_EQ(gesture.pixel, pointAt(100, 100));
    EXPECT_TRUE(gesture.ink);
    EXPECT_FALSE(gesture.ctrl);
    EXPECT_TRUE(store.input.down);
}

TEST_F(PointerSystemTest, Update_ReportsASheetPressWithPaperOnARightPress)
{
    store.view = EditorView::Tiles;

    feed({pressedAt(MouseButton::Right, 100, 100)});

    ASSERT_EQ(store.input.sheetGestures.size(), 1U);
    EXPECT_FALSE(store.input.sheetGestures.front().ink);
    EXPECT_FALSE(store.input.down);
}

TEST_F(PointerSystemTest, Update_MarksASheetPressMadeWithControlHeld)
{
    store.view = EditorView::Tiles;

    feed({pressedAt(MouseButton::Left, 100, 100, true)});

    ASSERT_EQ(store.input.sheetGestures.size(), 1U);
    EXPECT_TRUE(store.input.sheetGestures.front().ctrl);
}

TEST_F(PointerSystemTest, Update_ReportsNoSheetPressOverTheUi)
{
    store.view = EditorView::Tiles;
    store.ui.pointerOverUi = true;

    feed({pressedAt(MouseButton::Left, 100, 100)});

    EXPECT_TRUE(store.input.sheetGestures.empty());
    EXPECT_TRUE(store.input.pressed);
}

TEST_F(PointerSystemTest, Update_ReportsNoSheetPressUnderAnOpenMenu)
{
    store.view = EditorView::Tiles;
    store.ui.openMenu = 0;

    feed({pressedAt(MouseButton::Left, 100, 100)});

    EXPECT_TRUE(store.input.sheetGestures.empty());
}

TEST_F(PointerSystemTest, Update_ReportsNoSheetPressWhileAModalIsOpen)
{
    store.view = EditorView::Tiles;
    store.newTileset.open = true;

    feed({pressedAt(MouseButton::Left, 100, 100)});

    EXPECT_TRUE(store.input.sheetGestures.empty());
}

TEST_F(PointerSystemTest, Update_ReportsNoSheetPressOffTheTilesCanvas)
{
    store.view = EditorView::Tiles;

    feed({pressedAt(MouseButton::Left, 100, 9)});

    EXPECT_TRUE(store.input.sheetGestures.empty());
    EXPECT_TRUE(store.input.down);
}

TEST_F(PointerSystemTest, Update_IgnoresAnExtraButtonPressInTheTilesView)
{
    store.view = EditorView::Tiles;

    feed({pressedAt(MouseButton::X1, 100, 100)});

    EXPECT_TRUE(store.input.sheetGestures.empty());
    EXPECT_FALSE(store.input.down);
}

TEST_F(PointerSystemTest, Update_ReportsASheetReleaseOnALeftRelease)
{
    store.view = EditorView::Tiles;
    store.input.down = true;

    feed({releasedOf(MouseButton::Left)});

    EXPECT_FALSE(store.input.down);
    ASSERT_EQ(store.input.sheetGestures.size(), 1U);
    EXPECT_EQ(
        store.input.sheetGestures.front().kind, GestureKind::Release);
}

TEST_F(PointerSystemTest, Update_ReportsASheetReleaseOnARightRelease)
{
    store.view = EditorView::Tiles;
    store.input.down = true;

    feed({releasedOf(MouseButton::Right)});

    EXPECT_TRUE(store.input.down);
    ASSERT_EQ(store.input.sheetGestures.size(), 1U);
    EXPECT_EQ(
        store.input.sheetGestures.front().kind, GestureKind::Release);
}

TEST_F(PointerSystemTest, Update_IgnoresAnExtraButtonReleaseInTilesView)
{
    store.view = EditorView::Tiles;

    feed({releasedOf(MouseButton::X1)});

    EXPECT_TRUE(store.input.sheetGestures.empty());
}

TEST_F(PointerSystemTest, Update_ReportsASheetMoveOverTheCharacterSheet)
{
    store.view = EditorView::Characters;

    feed({movedTo(40, 20)});

    ASSERT_EQ(store.input.sheetGestures.size(), 1U);
    EXPECT_EQ(
        store.input.sheetGestures.front().pixel, pointAt(2, 2));
}

TEST_F(PointerSystemTest, Update_ReportsNoSheetMoveOffTheCharacterSheet)
{
    store.view = EditorView::Characters;

    feed({movedTo(10, 100)});

    EXPECT_TRUE(store.input.sheetGestures.empty());
}
