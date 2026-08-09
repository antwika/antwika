#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <array>
#include <chrono>
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

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
#include <antwika/console/ConsolePicture.hpp>

#include "antwika/atlas_editor/AtlasMeta.hpp"
#include "antwika/atlas_editor/Canvas.hpp"
#include "antwika/atlas_editor/CanvasView.hpp"
#include "antwika/atlas_editor/EditorScene.hpp"
#include "antwika/atlas_editor/EditorState.hpp"
#include "antwika/atlas_editor/Pixel.hpp"
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
using antwika::atlas_editor::Gesture;
using antwika::atlas_editor::imageRect;
using antwika::atlas_editor::Pixel;
using antwika::atlas_editor::pixelRect;
using antwika::atlas_editor::RenderSink;
using antwika::atlas_editor::SceneSnapshot;
using antwika::atlas_editor::Selection;
using antwika::atlas_editor::snapshotOf;
using antwika::atlas_editor::SpriteGuides;
using antwika::atlas_editor::TileGrid;
using antwika::atlas_editor::Tool;
using antwika::atlas_editor::UiOverlay;
using antwika::event::Event;
using antwika::event::TickEvent;
using antwika::gfx::Bitmap;
using antwika::gfx::Color;
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

    constexpr Size kGuidedSheet{.width = 128, .height = 96};
    constexpr TileGrid kGameSlots{.width = 64, .height = 96};
    constexpr SpriteGuides kGameSlotGuides{
        .pivot = {.x = 32, .y = 64},
        .footprint = {.width = 32, .height = 16}};

    SceneSnapshot guided(
        const std::optional<SpriteGuides> guides,
        const std::optional<Point> pivot = std::nullopt)
    {
        return SceneSnapshot{
            .image = kGuidedSheet,
            .view = CanvasView{.pan = {}, .zoom = 0},
            .tiles = kGameSlots,
            .gridVisible = false,
            .guides = guides,
            .pivot = pivot,
            .hovered = std::nullopt};
    }
}

TEST(EditorSceneTest, Draw_ClearsAndBacksTheSheetBeforeAnythingElse)
{
    NiceMock<MockRenderer> renderer;
    const EditorScene scene;
    const auto shown = snapshot();

    EXPECT_CALL(renderer, clear(_)).Times(1);
    EXPECT_CALL(renderer, drawRect(_, _)).Times(AnyNumber());
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

    EXPECT_CALL(renderer, drawLine(_, _, _)).Times(6);

    scene.draw(renderer, shown, nullptr);
}

TEST(EditorSceneTest, Draw_RulesALineBetweenEveryPixelOfTheSheet)
{
    NiceMock<MockRenderer> renderer;
    const EditorScene scene;

    auto shown = snapshot();
    shown.pixelGridVisible = true;
    shown.view = CanvasView{.pan = {}, .zoom = 3};

    const auto step = static_cast<std::int32_t>(
        antwika::atlas_editor::scaleOf(shown.view));

    EXPECT_CALL(renderer, drawLine(_, _, _)).Times(AnyNumber());
    EXPECT_CALL(
        renderer,
        drawLine(
            Point{.x = step, .y = 0},
            Point{
                .x = step,
                .y = static_cast<std::int32_t>(kSheet.height) * step},
            _))
        .Times(1);

    scene.draw(renderer, shown, nullptr);
}

TEST(EditorSceneTest, Draw_RulesOneLinePerPixelBoundaryOnBothAxes)
{
    NiceMock<MockRenderer> renderer;
    const EditorScene scene;

    auto shown = snapshot();
    shown.pixelGridVisible = true;
    shown.view = CanvasView{.pan = {}, .zoom = 3};

    EXPECT_CALL(renderer, drawLine(_, _, _))
        .Times(kSheet.width + kSheet.height + 2);

    scene.draw(renderer, shown, nullptr);
}

TEST(EditorSceneTest, Draw_RulesNoPixelLinesUntilThereIsRoomForThem)
{
    NiceMock<MockRenderer> renderer;
    const EditorScene scene;

    auto shown = snapshot();
    shown.pixelGridVisible = true;
    shown.view = CanvasView{.pan = {}, .zoom = 1};

    EXPECT_CALL(renderer, drawLine(_, _, _)).Times(0);

    scene.draw(renderer, shown, nullptr);
}

TEST(EditorSceneTest, Draw_DrawsNoDiamondsWhileTheGuidesAreOff)
{
    NiceMock<MockRenderer> renderer;
    const EditorScene scene;

    EXPECT_CALL(renderer, drawLine(_, _, _)).Times(0);

    scene.draw(renderer, guided(std::nullopt), nullptr);
}

TEST(EditorSceneTest, Draw_DrawsADiamondAndAPivotInEveryWholeSlot)
{
    NiceMock<MockRenderer> renderer;
    const EditorScene scene;

    constexpr int kStepsRoundTheDiamond = 68;
    constexpr int kPivotArms = 2;
    constexpr int kWholeSlots = 2;

    EXPECT_CALL(renderer, drawLine(_, _, _))
        .Times((kStepsRoundTheDiamond + kPivotArms) * kWholeSlots);

    scene.draw(
        renderer,
        guided(kGameSlotGuides, kGameSlotGuides.pivot),
        nullptr);
}

TEST(EditorSceneTest, Draw_TracesTheDiamondRoundTheTileItsSlotHolds)
{
    NiceMock<MockRenderer> renderer;
    const EditorScene scene;

    EXPECT_CALL(renderer, drawLine(_, _, _)).Times(AnyNumber());
    EXPECT_CALL(
        renderer,
        drawLine(Point{.x = 33, .y = 64}, Point{.x = 33, .y = 63}, _))
        .Times(1);
    EXPECT_CALL(
        renderer,
        drawLine(Point{.x = 48, .y = 56}, Point{.x = 48, .y = 55}, _))
        .Times(1);
    EXPECT_CALL(
        renderer,
        drawLine(Point{.x = 33, .y = 47}, Point{.x = 31, .y = 47}, _))
        .Times(1);
    EXPECT_CALL(
        renderer,
        drawLine(Point{.x = 16, .y = 55}, Point{.x = 16, .y = 56}, _))
        .Times(1);

    scene.draw(renderer, guided(kGameSlotGuides), nullptr);
}

TEST(EditorSceneTest, Draw_PokesTheDiamondOutOnePixelAtEachSideVertex)
{
    NiceMock<MockRenderer> renderer;
    const EditorScene scene;

    EXPECT_CALL(renderer, drawLine(_, _, _)).Times(AnyNumber());
    EXPECT_CALL(
        renderer,
        drawLine(Point{.x = 47, .y = 56}, Point{.x = 48, .y = 56}, _))
        .Times(1);
    EXPECT_CALL(
        renderer,
        drawLine(Point{.x = 17, .y = 55}, Point{.x = 16, .y = 55}, _))
        .Times(1);

    scene.draw(renderer, guided(kGameSlotGuides), nullptr);
}

TEST(EditorSceneTest, Draw_StepsTheGuidesAlongThePixelEdgesOnly)
{
    NiceMock<MockRenderer> renderer;
    const EditorScene scene;

    int legs = 0;
    int diagonals = 0;

    ON_CALL(renderer, drawLine(_, _, _))
        .WillByDefault(
            [&legs, &diagonals](
                const Point from, const Point to, const Color)
            {
                ++legs;

                if (from.x != to.x && from.y != to.y)
                {
                    ++diagonals;
                }
            });

    scene.draw(renderer, guided(kGameSlotGuides), nullptr);

    ASSERT_GT(legs, 0);
    EXPECT_EQ(diagonals, 0);
}

TEST(EditorSceneTest, Draw_CrossesThePivotUnderTheDiamondItAnchors)
{
    NiceMock<MockRenderer> renderer;
    const EditorScene scene;

    EXPECT_CALL(renderer, drawLine(_, _, _)).Times(AnyNumber());
    EXPECT_CALL(
        renderer,
        drawLine(Point{.x = 30, .y = 64}, Point{.x = 34, .y = 64}, _))
        .Times(1);
    EXPECT_CALL(
        renderer,
        drawLine(Point{.x = 32, .y = 62}, Point{.x = 32, .y = 66}, _))
        .Times(1);

    scene.draw(
        renderer,
        guided(kGameSlotGuides, kGameSlotGuides.pivot),
        nullptr);
}

TEST(EditorSceneTest, Draw_SitsThePivotHalfAPixelLeftAndAboveItsCentre)
{
    NiceMock<MockRenderer> renderer;
    const EditorScene scene;

    auto shown = guided(kGameSlotGuides, kGameSlotGuides.pivot);
    shown.view = CanvasView{.pan = {}, .zoom = 1};

    const auto step = static_cast<std::int32_t>(
        antwika::atlas_editor::scaleOf(shown.view));

    EXPECT_CALL(renderer, drawLine(_, _, _)).Times(AnyNumber());
    EXPECT_CALL(
        renderer,
        drawLine(
            Point{.x = 30 * step, .y = 64 * step},
            Point{.x = 34 * step, .y = 64 * step},
            _))
        .Times(1);

    scene.draw(renderer, shown, nullptr);
}

TEST(EditorSceneTest, Draw_ShowsThePixelsAShapeDragWouldPaint)
{
    NiceMock<MockRenderer> renderer;
    const EditorScene scene;

    auto shown = snapshot();
    shown.tool = Tool::Line;
    shown.stroke = Gesture{
        .carrying = false,
        .from = Pixel{.x = 1, .y = 1},
        .to = Pixel{.x = 3, .y = 1}};

    EXPECT_CALL(renderer, drawRect(_, _)).Times(AnyNumber());
    EXPECT_CALL(
        renderer,
        drawRect(pixelRect(shown.view, Pixel{.x = 2, .y = 1}), _))
        .Times(1);

    scene.draw(renderer, shown, nullptr);
}

TEST(EditorSceneTest, Draw_ShowsNoShapeWithNoDragInHand)
{
    NiceMock<MockRenderer> renderer;
    const EditorScene scene;

    auto shown = snapshot();
    shown.tool = Tool::Line;

    EXPECT_CALL(renderer, drawRect(_, _)).Times(AnyNumber());
    EXPECT_CALL(
        renderer,
        drawRect(pixelRect(shown.view, Pixel{.x = 2, .y = 1}), _))
        .Times(0);

    scene.draw(renderer, shown, nullptr);
}

TEST(EditorSceneTest, Draw_ScalesAndPansTheDiamondsWithTheSheet)
{
    NiceMock<MockRenderer> renderer;
    const EditorScene scene;

    auto shown = guided(kGameSlotGuides);
    shown.view = CanvasView{.pan = {.x = 5, .y = 7}, .zoom = 1};

    const auto step = static_cast<std::int32_t>(
        antwika::atlas_editor::scaleOf(shown.view));

    EXPECT_CALL(renderer, drawLine(_, _, _)).Times(AnyNumber());
    EXPECT_CALL(
        renderer,
        drawLine(
            Point{.x = (16 * step) + 5, .y = (55 * step) + 7},
            Point{.x = (16 * step) + 5, .y = (56 * step) + 7},
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

    sink.handle(tickAt(2));

    state.applyAt(state.view().pan);
    sink.handle(tickAt(3));

    EXPECT_EQ(sleeper.requested().size(), 3U);
}

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

    ASSERT_EQ(state.image().revision(), 0U);

    state.replace(Canvas::blank(Size{.width = 4, .height = 4}));
    sink.handle(tickAt(2));

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

TEST(EditorSceneTest, Draw_MarksThePivotWithTheDiamondTurnedOff)
{
    NiceMock<MockRenderer> renderer;
    const EditorScene scene;

    EXPECT_CALL(renderer, drawLine(_, _, _)).Times(AnyNumber());
    EXPECT_CALL(
        renderer,
        drawLine(Point{.x = 32, .y = 62}, Point{.x = 32, .y = 66}, _))
        .Times(1);

    scene.draw(
        renderer,
        guided(std::nullopt, kGameSlotGuides.pivot),
        nullptr);
}

TEST(EditorSceneTest, Draw_DrawsOnlyTwoArmsPerSlotForThePivotAlone)
{
    NiceMock<MockRenderer> renderer;
    const EditorScene scene;

    constexpr int kPivotArms = 2;
    constexpr int kWholeSlots = 2;

    EXPECT_CALL(renderer, drawLine(_, _, _))
        .Times(kPivotArms * kWholeSlots);

    scene.draw(
        renderer,
        guided(std::nullopt, kGameSlotGuides.pivot),
        nullptr);
}

TEST(EditorSceneTest, Draw_LeavesThePivotOffWhileOnlyTheDiamondIsOn)
{
    NiceMock<MockRenderer> renderer;
    const EditorScene scene;

    EXPECT_CALL(renderer, drawLine(_, _, _)).Times(AnyNumber());
    EXPECT_CALL(
        renderer,
        drawLine(Point{.x = 32, .y = 62}, Point{.x = 32, .y = 66}, _))
        .Times(0);

    scene.draw(renderer, guided(kGameSlotGuides), nullptr);
}

TEST(EditorSceneTest, Draw_ChequersTheSheetBehindWhatItHolds)
{
    NiceMock<MockRenderer> renderer;
    const EditorScene scene;

    auto shown = snapshot();
    shown.view = CanvasView{.pan = {}, .zoom = 0};

    EXPECT_CALL(renderer, drawRect(_, _)).Times(AnyNumber());
    EXPECT_CALL(
        renderer,
        drawRect(
            Rect{
                .origin = {.x = 0, .y = 0},
                .size = {.width = 8, .height = 8}},
            _))
        .Times(1);
    EXPECT_CALL(
        renderer,
        drawRect(
            Rect{
                .origin = {.x = 8, .y = 0},
                .size = {.width = 8, .height = 8}},
            _))
        .Times(0);

    scene.draw(renderer, shown, nullptr);
}

TEST(EditorSceneTest, Draw_ChequersOnlyAsFarAsTheSheetReaches)
{
    NiceMock<MockRenderer> renderer;
    const EditorScene scene;

    auto shown = snapshot();
    shown.view = CanvasView{.pan = {}, .zoom = 0};
    shown.image = Size{.width = 12, .height = 4};

    EXPECT_CALL(renderer, drawRect(_, _)).Times(AnyNumber());
    EXPECT_CALL(
        renderer,
        drawRect(
            Rect{
                .origin = {.x = 0, .y = 0},
                .size = {.width = 8, .height = 4}},
            _))
        .Times(1);

    scene.draw(renderer, shown, nullptr);
}

TEST(EditorSceneTest, Draw_RingsThePixelUnderThePointerWhenAskedTo)
{
    NiceMock<MockRenderer> renderer;
    const EditorScene scene;

    auto shown = snapshot();
    shown.hovered = Pixel{.x = 2, .y = 1};
    shown.pointerBorder = true;

    std::vector<std::pair<Point, Point>> legs;

    ON_CALL(renderer, drawLine(_, _, _))
        .WillByDefault(
            [&legs](const Point from, const Point to, const Color)
            { legs.emplace_back(from, to); });

    scene.draw(renderer, shown, nullptr);

    const Rect cell = pixelRect(shown.view, *shown.hovered);
    const Point corner = cell.origin;
    const Point far{
        .x = corner.x + static_cast<std::int32_t>(cell.size.width),
        .y = corner.y + static_cast<std::int32_t>(cell.size.height)};

    EXPECT_EQ(
        legs,
        (std::vector<std::pair<Point, Point>>{
            {corner, Point{.x = far.x, .y = corner.y}},
            {Point{.x = far.x, .y = corner.y}, far},
            {far, Point{.x = corner.x, .y = far.y}},
            {Point{.x = corner.x, .y = far.y}, corner}}));
}

TEST(EditorSceneTest, Draw_LeavesThePointerUnringedWhileItIsTurnedOff)
{
    NiceMock<MockRenderer> renderer;
    const EditorScene scene;

    auto shown = snapshot();
    shown.hovered = Pixel{.x = 2, .y = 1};
    shown.pointerBorder = false;

    EXPECT_CALL(renderer, drawLine(_, _, _)).Times(0);

    scene.draw(renderer, shown, nullptr);
}

TEST(SnapshotOfTest, SnapshotOf_CarriesThePivotOnceItIsShown)
{
    EditorState state = opened();

    EXPECT_FALSE(snapshotOf(state).pivot.has_value());

    state.togglePivot();

    EXPECT_EQ(snapshotOf(state).pivot, state.meta().pivot);
}

TEST(SnapshotOfTest, SnapshotOf_CarriesWhetherThePointerIsRinged)
{
    EditorState state = opened();

    EXPECT_TRUE(snapshotOf(state).pointerBorder);

    state.togglePointerBorder();

    EXPECT_FALSE(snapshotOf(state).pointerBorder);
}

TEST(SceneTest, SnapshotOf_TakesNoPreviewWhenTheStateHasNoPane)
{
    EditorState state(
        Canvas::blank(Size{.width = 16, .height = 16}),
        TileGrid{.width = 8, .height = 8},
        Size{.width = 200, .height = 200});

    state.togglePreview();

    EXPECT_FALSE(snapshotOf(state, std::nullopt).preview.has_value());
}

TEST(SceneTest, SnapshotOf_MarksNoSlotBeforeAnEditHasLandedInOne)
{
    EditorState state(
        Canvas::blank(Size{.width = 16, .height = 16}),
        TileGrid{.width = 8, .height = 8},
        Size{.width = 200, .height = 200});

    state.togglePreview();

    const Rect pane{
        .origin = {.x = 0, .y = 0},
        .size = {.width = 100, .height = 100}};

    const auto shot = snapshotOf(state, pane).preview;

    ASSERT_TRUE(shot.has_value());
    EXPECT_FALSE(shot->slot.has_value());
}

TEST(SceneTest, SnapshotOf_MarksNoSlotWhenTheGridHasNoColumns)
{
    EditorState state(
        Canvas::blank(Size{.width = 16, .height = 16}),
        TileGrid{.width = 8, .height = 8},
        Size{.width = 200, .height = 200});

    state.togglePreview();
    state.noteTouched(Pixel{.x = 1, .y = 1});

    state.adoptMeta(antwika::atlas_editor::AtlasMeta{
        .columns = 1, .rows = 1, .sprite = {.width = 64, .height = 64}});

    const Rect pane{
        .origin = {.x = 0, .y = 0},
        .size = {.width = 100, .height = 100}};

    const auto shot = snapshotOf(state, pane).preview;

    ASSERT_TRUE(shot.has_value());
    EXPECT_FALSE(shot->slot.has_value());
}
