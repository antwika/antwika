#include "antwika/music_editor/EditorScene.hpp"

#include <array>
#include <cstddef>
#include <string>

#include <gtest/gtest.h>

#include <antwika/gfx/Size.hpp>
#include <antwika/ui/Keyboard.hpp>
#include <antwika/ui/Pointer.hpp>

#include "antwika/music_editor/EditorState.hpp"
#include "antwika/music_editor/Score.hpp"
#include "antwika/music_editor/TrackPreset.hpp"

using antwika::gfx::Size;
using antwika::music_editor::EditorScene;
using antwika::music_editor::EditorState;
using antwika::music_editor::fieldFor;
using antwika::music_editor::kPanicButton;
using antwika::music_editor::kPlayButton;
using antwika::music_editor::kTrackCount;
using antwika::music_editor::openingState;
using antwika::music_editor::PlaybackStatus;
using antwika::music_editor::Score;
using antwika::music_editor::trackName;
using antwika::ui::Keyboard;
using antwika::ui::Pointer;

namespace
{
    constexpr Size kCanvas{.width = 960, .height = 420};

    [[nodiscard]] antwika::ui::Frame describe(
        const EditorState &state, const Score &score)
    {
        const EditorScene scene;

        return scene.describe(
            state, score, PlaybackStatus{}, kCanvas, Pointer{},
            Keyboard{});
    }
} // namespace

TEST(EditorSceneTest, EveryTrackHasAName)
{
    for (std::size_t track = 0; track < kTrackCount; ++track)
    {
        EXPECT_FALSE(trackName(track).empty()) << track;
    }
}

TEST(EditorSceneTest, DrawsSomethingForEveryLineAndBothButtons)
{
    const auto state = openingState();
    const Score score;

    const auto frame = describe(state, score);

    EXPECT_FALSE(frame.commands.empty());

    for (std::size_t track = 0; track < kTrackCount; ++track)
    {
        EXPECT_TRUE(frame.rects.find(fieldFor(track)).has_value())
            << track;
    }

    EXPECT_TRUE(frame.rects.find(kPlayButton).has_value());
    EXPECT_TRUE(frame.rects.find(kPanicButton).has_value());
}

// The same state always describes the same picture.
// That is what lets a layout be asserted with no window.
TEST(EditorSceneTest, IsDeterministic)
{
    const auto state = openingState();
    const Score score;

    EXPECT_EQ(describe(state, score).commands,
              describe(state, score).commands);
}

TEST(EditorSceneTest, SaysWhetherItIsPlayingOrPaused)
{
    auto state = openingState();
    const Score score;

    const auto playing = describe(state, score).commands;

    state.paused = true;
    const auto paused = describe(state, score).commands;

    EXPECT_NE(playing, paused);
}

TEST(EditorSceneTest, ShowsALineThatWasRefused)
{
    auto state = openingState();
    state.lines[1] = "0 [";

    Score score;
    score.update(state.lines);
    ASSERT_TRUE(score.hasError());

    const Score clean;

    EXPECT_NE(
        describe(state, score).commands,
        describe(state, clean).commands);
}

TEST(EditorSceneTest, MovingTheFocusChangesThePicture)
{
    auto state = openingState();
    const Score score;

    const auto first = describe(state, score).commands;

    state.focused = 2;

    EXPECT_NE(describe(state, score).commands, first);
}

TEST(EditorSceneTest, WhatIsPlayingIsSaidRatherThanReachedFor)
{
    const auto state = openingState();
    const Score score;
    const EditorScene scene;

    const auto quiet = scene.describe(
        state, score, PlaybackStatus{}, kCanvas, Pointer{}, Keyboard{});

    const auto busy = scene.describe(
        state,
        score,
        PlaybackStatus{.started = 12, .voices = 3, .cycles = 4},
        kCanvas,
        Pointer{},
        Keyboard{});

    EXPECT_NE(quiet.commands, busy.commands);
}
