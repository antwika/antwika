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
#include "antwika/atlas_editor/RenderSink.hpp"
#include "antwika/atlas_editor/SceneSnapshot.hpp"
#include "antwika/atlas_editor/TileGrid.hpp"
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
using antwika::atlas_editor::snapshotOf;
using antwika::atlas_editor::TileGrid;
using antwika::atlas_editor::UiOverlay;
using antwika::event::Event;
using antwika::event::TickEvent;
using antwika::gfx::Bitmap;
using antwika::gfx::mocks::MockRenderer;
using antwika::gfx::mocks::MockTexture;
using antwika::gfx::mocks::MockWindow;
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

    RenderSink sink(
        window, scene, state, overlay, sleeper, kFramePeriod);

    sink.handle(tickAt(1));

    // Nothing changed, so nothing is uploaded again.
    sink.handle(tickAt(2));

    state.applyAt(state.view().pan);
    sink.handle(tickAt(3));

    EXPECT_EQ(sleeper.requested().size(), 3U);
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

    RenderSink sink(
        window, scene, state, overlay, sleeper, kFramePeriod);

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

    RenderSink sink(
        window, scene, state, overlay, sleeper, kFramePeriod);

    sink.handle(TickEvent{
        .tick = 1, .event = Event{.name = "atlas_editor.nothing"}});
}
