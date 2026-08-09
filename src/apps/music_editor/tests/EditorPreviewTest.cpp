#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <antwika/app/preview/DrawnPreview.hpp>
#include <antwika/gfx/IRenderer.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/ui/Keyboard.hpp>
#include <antwika/ui/Pointer.hpp>

#include "antwika/music_editor/EditorScene.hpp"
#include "antwika/music_editor/EditorState.hpp"
#include "antwika/music_editor/Score.hpp"

namespace
{
    using antwika::app::preview::drawnPreview;
    using antwika::gfx::IRenderer;
    using antwika::gfx::Size;
    using antwika::music_editor::EditorScene;
    using antwika::music_editor::EditorState;
    using antwika::music_editor::PlaybackStatus;
    using antwika::music_editor::Score;
    using antwika::ui::Keyboard;
    using antwika::ui::Pointer;

    constexpr Size kCanvas{.width = 1024, .height = 720};
}

TEST(EditorPreviewTest, Draw_WritesTheEditorOverAScore)
{
    EXPECT_FALSE(
        drawnPreview(
            {.name = "music-editor",
             .title = "Antwika Music Editor",
             .canvas = kCanvas},
            [](IRenderer &renderer)
            {
                const EditorScene scene;
                const EditorState state;
                const Score score;
                const Keyboard keyboard;

                const auto frame = scene.describe(
                    state,
                    score,
                    PlaybackStatus{},
                    kCanvas,
                    Pointer{},
                    keyboard);

                scene.draw(renderer, frame.commands);
            })
            .empty());
}
