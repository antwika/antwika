#include "antwika/music_editor/EditorScene.hpp"

#include <algorithm>
#include <cstddef>
#include <string>
#include <variant>
#include <vector>

#include <gtest/gtest.h>

#include <antwika/ui/DrawCommand.hpp>
#include <antwika/ui/Frame.hpp>
#include <antwika/ui/Keyboard.hpp>
#include <antwika/ui/Pointer.hpp>

#include "antwika/music_editor/EditorState.hpp"
#include "antwika/music_editor/Score.hpp"
#include "EditorRig.hpp"

using antwika::music_editor::editorTheme;
using antwika::music_editor::EditorScene;
using antwika::music_editor::EditorState;
using antwika::music_editor::kCodeField;
using antwika::music_editor::kPanicButton;
using antwika::music_editor::kPlayButton;
using antwika::music_editor::kTextScale;
using antwika::music_editor::openingState;
using antwika::music_editor::PlaybackStatus;
using antwika::music_editor::Score;
using antwika::music_editor::tests::kCanvas;
using antwika::ui::DrawList;
using antwika::ui::DrawText;
using antwika::ui::Keyboard;
using antwika::ui::Pointer;

namespace
{
    [[nodiscard]] antwika::ui::Frame describe(
        const EditorState &state,
        const Score &score,
        const PlaybackStatus &status = PlaybackStatus{})
    {
        const EditorScene scene;

        return scene.describe(
            state, score, status, kCanvas, Pointer{}, Keyboard{});
    }

    // Every line of text the picture holds, in the order it is drawn.
    [[nodiscard]] std::vector<std::string> words(const DrawList &picture)
    {
        std::vector<std::string> said;

        for (const auto &command : picture)
        {
            if (const auto *text = std::get_if<DrawText>(&command))
            {
                said.push_back(text->text);
            }
        }

        return said;
    }

    [[nodiscard]] bool says(
        const DrawList &picture, const std::string &what)
    {
        for (const auto &line : words(picture))
        {
            if (line.find(what) != std::string::npos)
            {
                return true;
            }
        }

        return false;
    }

    // A document whose every line is refused.
    // So a test asks for exactly as many problems as it wants.
    [[nodiscard]] EditorState refusedLines(std::size_t count)
    {
        EditorState state;

        for (std::size_t line = 0; line < count; ++line)
        {
            state.source += "no\n";
        }

        return state;
    }
} // namespace

// What is being read here is code.
// A mis-read bracket is a line that will not play.
TEST(EditorSceneTest, DrawsItsTextAtTwiceTheGlyphScale)
{
    EXPECT_EQ(kTextScale, 2U);
    EXPECT_EQ(editorTheme().textScale, kTextScale);

    const auto state = openingState();
    const Score score;

    for (const auto &command : describe(state, score).commands)
    {
        if (const auto *text = std::get_if<DrawText>(&command))
        {
            EXPECT_EQ(text->scale, kTextScale) << text->text;
        }
    }
}

TEST(EditorSceneTest, DrawsOneCodePaneAndBothButtons)
{
    const auto state = openingState();
    const Score score;

    const auto frame = describe(state, score);

    EXPECT_FALSE(frame.commands.empty());
    EXPECT_TRUE(frame.rects.find(kCodeField).has_value());
    EXPECT_TRUE(frame.rects.find(kPlayButton).has_value());
    EXPECT_TRUE(frame.rects.find(kPanicButton).has_value());
}

// The document is drawn a line at a time.
// So what is on screen keeps the shape of what was typed.
TEST(EditorSceneTest, DrawsEveryLineOfTheDocument)
{
    EditorState state;
    state.source = "$: bass.n(\"0\")\n$: lead.n(\"3\")\n";

    const Score score;
    const auto said = words(describe(state, score).commands);

    EXPECT_NE(
        std::find(said.begin(), said.end(), "$: bass.n(\"0\")"),
        said.end());

    EXPECT_NE(
        std::find(said.begin(), said.end(), "$: lead.n(\"3\")"),
        said.end());
}

// The same state always describes the same picture.
// That is what lets a layout be asserted with no window.
TEST(EditorSceneTest, IsDeterministic)
{
    const auto state = openingState();
    const Score score;

    EXPECT_EQ(
        describe(state, score).commands,
        describe(state, score).commands);
}

TEST(EditorSceneTest, SaysWhetherItIsPlayingOrPaused)
{
    auto state = openingState();
    const Score score;

    const auto playing = describe(state, score).commands;
    EXPECT_TRUE(says(playing, "playing"));

    state.paused = true;
    const auto paused = describe(state, score).commands;

    EXPECT_TRUE(says(paused, "paused"));
    EXPECT_NE(playing, paused);

    // And the button offers the way back out.
    EXPECT_TRUE(says(playing, "pause"));
    EXPECT_TRUE(says(paused, "resume"));
}

TEST(EditorSceneTest, SaysNothingAboutLinesWhenNothingIsRefused)
{
    const auto state = openingState();
    Score score;
    score.read(state.source);
    ASSERT_FALSE(score.hasError());

    EXPECT_FALSE(says(describe(state, score).commands, "line "));
}

TEST(EditorSceneTest, NamesTheLineThatWasRefused)
{
    EditorState state;
    state.source = "$: bass.n(\"0\")\nnot a voice line\n";

    Score score;
    score.read(state.source);
    ASSERT_EQ(score.problems().size(), 1U);

    EXPECT_TRUE(says(describe(state, score).commands, "line 2: "));
}

// A document with more wrong with it than fits says so.
// Showing them all would push the code they are about off the window.
TEST(EditorSceneTest, ShowsAtMostThreeProblemsAndCountsTheRest)
{
    const auto three = refusedLines(3);
    Score readThree;
    readThree.read(three.source);
    ASSERT_EQ(readThree.problems().size(), 3U);

    const auto shown = describe(three, readThree).commands;

    EXPECT_TRUE(says(shown, "line 3: "));
    EXPECT_FALSE(says(shown, " more"));

    const auto five = refusedLines(5);
    Score readFive;
    readFive.read(five.source);
    ASSERT_EQ(readFive.problems().size(), 5U);

    const auto capped = describe(five, readFive).commands;

    EXPECT_TRUE(says(capped, "line 3: "));
    EXPECT_FALSE(says(capped, "line 4: "));
    EXPECT_TRUE(says(capped, "2 more"));
}

// Everything in the status line belongs to the projection side.
// A scene reaching for it would read the audio back into the picture.
TEST(EditorSceneTest, WhatIsPlayingIsSaidRatherThanReachedFor)
{
    const auto state = openingState();
    const Score score;

    const auto quiet = describe(state, score).commands;

    const auto busy = describe(
        state,
        score,
        PlaybackStatus{
            .started = 12, .voices = 3, .cycles = 4, .lines = 2});

    EXPECT_NE(quiet, busy.commands);
    EXPECT_TRUE(says(busy.commands, "cycle 4"));
    EXPECT_TRUE(says(busy.commands, "lines 2"));
    EXPECT_TRUE(says(busy.commands, "voices 3"));
    EXPECT_TRUE(says(busy.commands, "notes 12"));
}

// An emptied document still says what one line looks like.
TEST(EditorSceneTest, ShowsAnExampleWhileThereIsNothingToShow)
{
    EditorState state;
    const Score score;

    EXPECT_TRUE(says(describe(state, score).commands, "$: bass"));
}

TEST(EditorSceneTest, MovingTheCaretChangesThePicture)
{
    auto state = openingState();
    const Score score;

    const auto atEnd = describe(state, score).commands;

    state.cursor = 0;

    EXPECT_NE(describe(state, score).commands, atEnd);
}

TEST(EditorSceneTest, TheMenuIsAtTheTopAndClosedByDefault)
{
    const EditorState state = openingState();
    Score score;
    score.read(state.source);

    const auto frame = describe(state, score);

    EXPECT_TRUE(
        frame.rects.find(antwika::music_editor::kMenuBox).has_value());
    EXPECT_TRUE(says(frame.commands, "menu"));
    EXPECT_FALSE(says(frame.commands, "quit"));
}

TEST(EditorSceneTest, AnOpenMenuListsItsFourCommands)
{
    EditorState state = openingState();
    state.menuOpen = true;

    Score score;
    score.read(state.source);

    const auto frame = describe(state, score);

    EXPECT_TRUE(says(frame.commands, "new"));
    EXPECT_TRUE(says(frame.commands, "save"));
    EXPECT_TRUE(says(frame.commands, "load"));
    EXPECT_TRUE(says(frame.commands, "quit"));
}

TEST(EditorSceneTest, TheSaveBoxAsksForANameAndOffersBothButtons)
{
    const EditorScene scene;

    EditorState state = openingState();
    state.modal = antwika::music_editor::Modal::Save;
    state.fileName = "beat";

    const auto frame = scene.describeModal(
        state, kCanvas, Pointer{}, Keyboard{});

    EXPECT_TRUE(says(frame.commands, "save the score as"));
    EXPECT_TRUE(says(frame.commands, "beat"));
    EXPECT_TRUE(
        frame.rects.find(antwika::music_editor::kSaveConfirm)
            .has_value());
    EXPECT_TRUE(
        frame.rects.find(antwika::music_editor::kModalCancel)
            .has_value());
}

TEST(EditorSceneTest, TheLoadBoxListsEveryScoreAsAButton)
{
    const EditorScene scene;

    EditorState state = openingState();
    state.modal = antwika::music_editor::Modal::Load;
    state.scores = {"alpha", "zed"};

    const auto frame = scene.describeModal(
        state, kCanvas, Pointer{}, Keyboard{});

    EXPECT_TRUE(says(frame.commands, "load a score"));
    EXPECT_TRUE(says(frame.commands, "alpha"));
    EXPECT_TRUE(says(frame.commands, "zed"));
    EXPECT_TRUE(
        frame.rects.find(antwika::music_editor::loadOption(1))
            .has_value());

    // No save button in a box that only opens things.
    EXPECT_FALSE(
        frame.rects.find(antwika::music_editor::kSaveConfirm)
            .has_value());
}

TEST(EditorSceneTest, TheLoadBoxSaysWhenThereIsNothingToLoad)
{
    const EditorScene scene;

    EditorState state = openingState();
    state.modal = antwika::music_editor::Modal::Load;

    const auto frame = scene.describeModal(
        state, kCanvas, Pointer{}, Keyboard{});

    EXPECT_TRUE(says(frame.commands, "nothing saved yet"));
}

TEST(EditorSceneTest, ABoxShowsItsNoticeInItsOwnInk)
{
    const EditorScene scene;

    EditorState state = openingState();
    state.modal = antwika::music_editor::Modal::Save;
    state.notice = "name it first";

    const auto frame = scene.describeModal(
        state, kCanvas, Pointer{}, Keyboard{});

    EXPECT_TRUE(says(frame.commands, "name it first"));
}
