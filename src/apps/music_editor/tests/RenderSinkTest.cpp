#include "antwika/music_editor/RenderSink.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <antwika/engine/Events.hpp>
#include <antwika/event/TickEvent.hpp>
#include <antwika/gfx/Rect.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/gfx/mocks/MockRenderer.hpp>
#include <antwika/gfx/mocks/MockWindow.hpp>

#include "EditorRig.hpp"

using antwika::event::TickEvent;
using antwika::gfx::mocks::MockRenderer;
using antwika::gfx::mocks::MockWindow;
using antwika::music_editor::RenderSink;
using antwika::music_editor::tests::EditorRig;
using antwika::music_editor::tests::kCanvas;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::ReturnRef;

namespace
{
    [[nodiscard]] TickEvent tickAt(antwika::time::Tick when)
    {
        return TickEvent{
            .tick = when,
            .event = {.name = antwika::engine::events::kTick}};
    }
} // namespace

TEST(RenderSinkTest, DrawsAndPresentsOnTheTick)
{
    NiceMock<MockRenderer> renderer;
    NiceMock<MockWindow> window;

    EXPECT_CALL(window, isOpen()).WillRepeatedly(Return(true));
    EXPECT_CALL(window, renderer()).WillRepeatedly(ReturnRef(renderer));
    EXPECT_CALL(window, size()).WillRepeatedly(Return(kCanvas));
    EXPECT_CALL(renderer, present()).Times(1);

    EditorRig rig;
    RenderSink sink(
        window, rig.scene, rig.editor, kCanvas, rig.consolePicture);

    sink.handle(tickAt(1));
}

// Nothing but the tick draws, so a run's frame count is its tick count.
TEST(RenderSinkTest, DrawsNothingForAnyOtherEvent)
{
    NiceMock<MockRenderer> renderer;
    NiceMock<MockWindow> window;

    EXPECT_CALL(window, isOpen()).WillRepeatedly(Return(true));
    EXPECT_CALL(renderer, present()).Times(0);

    EditorRig rig;
    RenderSink sink(
        window, rig.scene, rig.editor, kCanvas, rig.consolePicture);

    sink.handle(
        TickEvent{.tick = 1, .event = {.name = "input.key_pressed"}});
}

// It never closes the window and never asks it anything else.
TEST(RenderSinkTest, DrawsNothingOnceTheWindowHasClosed)
{
    NiceMock<MockRenderer> renderer;
    NiceMock<MockWindow> window;

    EXPECT_CALL(window, isOpen()).WillRepeatedly(Return(false));
    EXPECT_CALL(renderer, present()).Times(0);

    EditorRig rig;
    RenderSink sink(
        window, rig.scene, rig.editor, kCanvas, rig.consolePicture);

    sink.handle(tickAt(1));
}

// Filling the screen is the reported size growing, and nothing else.
// The picture is scaled to the window's full height at its own aspect.
// The width left over is pillarboxed -- never more canvas.
TEST(RenderSinkTest, AWiderWindowIsPillarboxedAroundTheScaledCanvas)
{
    NiceMock<MockRenderer> renderer;
    NiceMock<MockWindow> window;

    // Twice the canvas's height and wider still than twice its width.
    constexpr antwika::gfx::Size kFullscreen{
        .width = 3360, .height = 1280};

    EXPECT_CALL(window, isOpen()).WillRepeatedly(Return(true));
    EXPECT_CALL(window, renderer()).WillRepeatedly(ReturnRef(renderer));
    EXPECT_CALL(window, size()).WillRepeatedly(Return(kFullscreen));
    EXPECT_CALL(renderer, present()).Times(1);

    // The doubled canvas is 2240 wide, so 560 is left either side.
    EXPECT_CALL(
        renderer,
        drawRect(
            antwika::gfx::Rect{
                .origin = {.x = 0, .y = 0},
                .size = {.width = 560, .height = 1280}},
            ::testing::_))
        .Times(1);

    EXPECT_CALL(
        renderer,
        drawRect(
            antwika::gfx::Rect{
                .origin = {.x = 2800, .y = 0},
                .size = {.width = 560, .height = 1280}},
            ::testing::_))
        .Times(1);

    EditorRig rig;
    RenderSink sink(
        window, rig.scene, rig.editor, kCanvas, rig.consolePicture);

    sink.handle(tickAt(1));
}
