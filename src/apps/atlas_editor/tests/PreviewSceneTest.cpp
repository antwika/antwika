#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <optional>

#include <antwika/gfx/Rect.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/gfx/mocks/MockRenderer.hpp>

#include "antwika/atlas_editor/CanvasView.hpp"
#include "antwika/atlas_editor/EditorScene.hpp"
#include "antwika/atlas_editor/Preview.hpp"
#include "antwika/atlas_editor/SceneSnapshot.hpp"

using antwika::atlas_editor::CanvasView;
using antwika::atlas_editor::EditorScene;
using antwika::atlas_editor::fittedView;
using antwika::atlas_editor::PreviewShot;
using antwika::atlas_editor::SceneSnapshot;
using antwika::gfx::mocks::MockRenderer;
using antwika::gfx::Rect;
using antwika::gfx::Size;
using ::testing::NiceMock;

namespace
{
    constexpr Size kSheet{.width = 32, .height = 32};

    constexpr Rect kPane{
        .origin = {.x = 200, .y = 20},
        .size = {.width = 120, .height = 120}};

    [[nodiscard]] SceneSnapshot shownWith(const PreviewShot &shot)
    {
        return SceneSnapshot{.image = kSheet, .preview = shot};
    }

    [[nodiscard]] Rect wholeSlot()
    {
        return Rect{
            .origin = {.x = 0, .y = 0},
            .size = {.width = 16, .height = 16}};
    }
}

TEST(PreviewSceneTest, Draw_PaintsThePaneWithNoSheetToBlit)
{
    NiceMock<MockRenderer> renderer;

    const EditorScene scene;

    scene.draw(
        renderer,
        shownWith(PreviewShot{
            .pane = kPane, .view = fittedView(kPane, wholeSlot())}),
        nullptr);
}

TEST(PreviewSceneTest, Draw_PaintsThePaneWithItsViewOffTheSheet)
{
    NiceMock<MockRenderer> renderer;

    const EditorScene scene;

    scene.draw(
        renderer,
        shownWith(PreviewShot{
            .pane = kPane,
            .view = CanvasView{.pan = {.x = 9000, .y = 9000}}}),
        nullptr);
}

TEST(PreviewSceneTest, Draw_MarksNoSlotThatReachesPastThePanesLeft)
{
    NiceMock<MockRenderer> renderer;

    const EditorScene scene;

    scene.draw(
        renderer,
        shownWith(PreviewShot{
            .pane = kPane,
            .view = CanvasView{.pan = {.x = 100, .y = 20}},
            .slot = wholeSlot()}),
        nullptr);
}

TEST(PreviewSceneTest, Draw_MarksNoSlotThatReachesPastThePanesTop)
{
    NiceMock<MockRenderer> renderer;

    const EditorScene scene;

    scene.draw(
        renderer,
        shownWith(PreviewShot{
            .pane = kPane,
            .view = CanvasView{.pan = {.x = 200, .y = 0}},
            .slot = wholeSlot()}),
        nullptr);
}

TEST(PreviewSceneTest, Draw_MarksNoSlotThatReachesPastThePanesRight)
{
    NiceMock<MockRenderer> renderer;

    const EditorScene scene;

    scene.draw(
        renderer,
        shownWith(PreviewShot{
            .pane = kPane,
            .view = CanvasView{.pan = {.x = 310, .y = 20}},
            .slot = wholeSlot()}),
        nullptr);
}

TEST(PreviewSceneTest, Draw_MarksNoSlotThatReachesPastThePanesFoot)
{
    NiceMock<MockRenderer> renderer;

    const EditorScene scene;

    scene.draw(
        renderer,
        shownWith(PreviewShot{
            .pane = kPane,
            .view = CanvasView{.pan = {.x = 200, .y = 130}},
            .slot = wholeSlot()}),
        nullptr);
}

TEST(PreviewSceneTest, Draw_MarksASlotThatSitsWhollyInThePane)
{
    NiceMock<MockRenderer> renderer;

    const EditorScene scene;

    scene.draw(
        renderer,
        shownWith(PreviewShot{
            .pane = kPane,
            .view = CanvasView{.pan = {.x = 210, .y = 30}},
            .slot = wholeSlot()}),
        nullptr);
}
