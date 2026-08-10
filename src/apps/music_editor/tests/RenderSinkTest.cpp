#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <antwika/engine/Events.hpp>
#include <antwika/event/TickEvent.hpp>
#include <antwika/gfx/Rect.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/gfx/mocks/MockRenderer.hpp>
#include <antwika/gfx/mocks/MockWindow.hpp>
#include <antwika/gfx/RectF.hpp>

#include "antwika/music_editor/RenderSink.hpp"
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
}

TEST(RenderSinkTest, Handle_DrawsAndPresentsOnTheTick)
{
    NiceMock<MockRenderer> renderer;
    NiceMock<MockWindow> window;

    EXPECT_CALL(window, isOpen()).WillRepeatedly(Return(true));
    EXPECT_CALL(window, renderer()).WillRepeatedly(ReturnRef(renderer));
    EXPECT_CALL(window, size()).WillRepeatedly(Return(kCanvas));
    EXPECT_CALL(renderer, clear(::testing::_)).Times(1);
    EXPECT_CALL(renderer, present()).Times(1);

    EditorRig rig;
    RenderSink sink(
        window, rig.scene, rig.editor, kCanvas, rig.consolePicture);

    sink.handle(tickAt(1));
}

TEST(RenderSinkTest, Handle_DrawsNothingForAnyOtherEvent)
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

TEST(RenderSinkTest, Handle_DrawsNothingOnceTheWindowHasClosed)
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

TEST(RenderSinkTest, Handle_AWiderWindowIsPillarboxedAroundTheScaledCanvas)
{
    NiceMock<MockRenderer> renderer;
    NiceMock<MockWindow> window;

    constexpr antwika::gfx::Size kFullscreen{
        .width = 3360, .height = 1280};

    EXPECT_CALL(window, isOpen()).WillRepeatedly(Return(true));
    EXPECT_CALL(window, renderer()).WillRepeatedly(ReturnRef(renderer));
    EXPECT_CALL(window, size()).WillRepeatedly(Return(kFullscreen));
    EXPECT_CALL(renderer, present()).Times(1);

    EXPECT_CALL(
        renderer,
        drawRect(
            antwika::gfx::RectF{antwika::gfx::Rect{
                .origin = {.x = 0, .y = 0},
                .size = {.width = 560, .height = 1280}}},
            ::testing::_))
        .Times(1);

    EXPECT_CALL(
        renderer,
        drawRect(
            antwika::gfx::RectF{antwika::gfx::Rect{
                .origin = {.x = 2800, .y = 0},
                .size = {.width = 560, .height = 1280}}},
            ::testing::_))
        .Times(1);

    EditorRig rig;
    RenderSink sink(
        window, rig.scene, rig.editor, kCanvas, rig.consolePicture);

    sink.handle(tickAt(1));
}
