#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <antwika/gfx/Size.hpp>
#include <antwika/gfx/mocks/MockRenderer.hpp>
#include <antwika/ui/Keyboard.hpp>
#include <antwika/ui/Pointer.hpp>

#include "antwika/music_editor/EditorScene.hpp"
#include "antwika/music_editor/EditorState.hpp"
#include "antwika/music_editor/Score.hpp"

namespace
{
    using antwika::gfx::Size;
    using antwika::gfx::mocks::MockRenderer;
    using antwika::music_editor::EditorScene;
    using antwika::music_editor::EditorState;
    using antwika::music_editor::PlaybackStatus;
    using antwika::music_editor::Score;
    using antwika::ui::Keyboard;
    using antwika::ui::Pointer;
    using ::testing::_;
    using ::testing::AtLeast;
    using ::testing::NiceMock;

    constexpr Size kCanvas{.width = 1024, .height = 720};
}

TEST(EditorDrawTest, Draw_DrawsTheEditorOverAScore)
{
    NiceMock<MockRenderer> renderer;

    EXPECT_CALL(renderer, drawRect(_, _)).Times(AtLeast(1));

    const EditorScene scene;
    const EditorState state;
    const Score score;
    const Keyboard keyboard;

    const auto frame = scene.describe(
        state, score, PlaybackStatus{}, kCanvas, Pointer{}, keyboard);

    scene.draw(renderer, frame.commands);
}
