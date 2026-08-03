#include "antwika/music_editor/EditorScene.hpp"

#include <algorithm>
#include <cstddef>
#include <string>
#include <variant>
#include <vector>

#include <gtest/gtest.h>

#include <antwika/gfx/Glyphs.hpp>
#include <antwika/gfx/Rect.hpp>
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

namespace
{
    using antwika::gfx::Color;
    using antwika::gfx::Rect;
    using antwika::music_editor::kPianorollRows;
    using antwika::music_editor::pianorollBand;
    using antwika::ui::FillRect;

    // The scene's own inks, as the picture is asserted against.
    constexpr Color kRollBackdrop{
        .red = 12, .green = 13, .blue = 18, .alpha = 255};

    constexpr Color kNoteInk{
        .red = 130, .green = 205, .blue = 140, .alpha = 255};

    // How tall a roll's band is, in canvas pixels.
    constexpr std::uint32_t kRollHeight =
        kPianorollRows * antwika::gfx::kGlyphLineHeight
        * antwika::music_editor::kTextScale;

    [[nodiscard]] std::vector<FillRect> fillsIn(
        const DrawList &picture, const Color color)
    {
        std::vector<FillRect> fills;

        for (const auto &command : picture)
        {
            const auto *fill =
                std::get_if<antwika::ui::FillRect>(&command);

            if (fill != nullptr && fill->color == color)
            {
                fills.push_back(*fill);
            }
        }

        return fills;
    }

    [[nodiscard]] antwika::ui::Frame pianorollFrame(
        const std::string &source, const std::size_t scroll = 0)
    {
        EditorState state;
        state.source = source;
        state.scroll = scroll;

        Score score;
        score.read(state.source);

        return describe(state, score);
    }
} // namespace

// The room is the band's, and the picture is painted over it.
// Both come off the one layout, so neither can drift from the other.
TEST(EditorSceneTest, APianorollLineHoldsABandOfRoomUnderItself)
{
    const auto frame = pianorollFrame(
        "$: drum.n(\"0 3\").pianoroll()\n"
        "$: bass.n(\"0\")\n");

    const auto band = frame.rects.find(pianorollBand(0));

    ASSERT_TRUE(band.has_value());
    EXPECT_EQ(band->size.height, kRollHeight);

    const auto backdrops = fillsIn(frame.commands, kRollBackdrop);

    ASSERT_EQ(backdrops.size(), 1U);
    EXPECT_EQ(backdrops[0].rect, *band);
}

// Time runs across the band as one cycle.
// Each distinct semitone is a lane, lowest at the bottom.
TEST(EditorSceneTest, PaintsTheRollsNotesOverItsBand)
{
    const auto frame =
        pianorollFrame("$: drum.n(\"0 3\").pianoroll()\n");

    const auto band = frame.rects.find(pianorollBand(0));

    ASSERT_TRUE(band.has_value());

    // Four lanes for pitches nought through three.
    const auto lane = kRollHeight / 4;
    const auto half = band->size.width / 2;

    const auto notes = fillsIn(frame.commands, kNoteInk);

    ASSERT_EQ(notes.size(), 2U);
    EXPECT_EQ(
        notes[0].rect,
        (Rect{
            .origin =
                {.x = band->origin.x,
                 .y = band->origin.y
                      + static_cast<std::int32_t>(kRollHeight - lane)},
            .size = {.width = half, .height = lane}}));
    EXPECT_EQ(
        notes[1].rect,
        (Rect{
            .origin =
                {.x = band->origin.x + static_cast<std::int32_t>(half),
                 .y = band->origin.y},
            .size =
                {.width = band->size.width - half, .height = lane}}));
}

TEST(EditorSceneTest, ARollOfRestsIsItsBackdropAlone)
{
    const auto frame =
        pianorollFrame("$: drum.n(\"~ ~\").pianoroll()\n");

    EXPECT_EQ(fillsIn(frame.commands, kRollBackdrop).size(), 1U);
    EXPECT_TRUE(fillsIn(frame.commands, kNoteInk).empty());
}

// A pattern can parse and still refuse a window.
// The roll comes out empty, exactly as the sound falls silent.
TEST(EditorSceneTest, ARollWhosePatternRefusesItsWindowIsEmpty)
{
    const auto frame = pianorollFrame(
        "$: bass.n(\"0/1000/1000/1000/1000/1000/1000/1000\")"
        ".pianoroll()\n");

    EXPECT_EQ(fillsIn(frame.commands, kRollBackdrop).size(), 1U);
    EXPECT_TRUE(fillsIn(frame.commands, kNoteInk).empty());
}

// A range of fifty-one semitones gets one-pixel lanes.
// The lanes past the band's top are not drawn rather than drawn wrong.
TEST(EditorSceneTest, ALanePastTheBandsTopIsNotDrawn)
{
    const auto frame =
        pianorollFrame("$: drum.n(\"0 50\").pianoroll()\n");

    const auto notes = fillsIn(frame.commands, kNoteInk);

    ASSERT_EQ(notes.size(), 1U);
    EXPECT_EQ(notes[0].rect.size.height, 1U);
}

TEST(EditorSceneTest, ARollScrolledOffTheTopPaintsNothing)
{
    std::string source = "$: drum.n(\"0\").pianoroll()\n";

    for (std::size_t line = 0; line < 40; ++line)
    {
        source += "// filler\n";
    }

    const auto frame = pianorollFrame(source, 3);

    EXPECT_FALSE(frame.rects.find(pianorollBand(0)).has_value());
    EXPECT_TRUE(fillsIn(frame.commands, kRollBackdrop).empty());
}

// The pane shows whole rows and no half ones, and a roll follows.
// A band cut short by the bottom edge paints nothing at all.
TEST(EditorSceneTest, ARollCutShortByThePanesBottomPaintsNothing)
{
    // Where the pane's rows land is the layout's answer.
    // So it is read off a first frame rather than worked out here.
    const auto sized = pianorollFrame("$: drum.n(\"0\")\n");
    const auto pane = sized.rects.find(kCodeField);

    ASSERT_TRUE(pane.has_value());

    const auto inner = pane->size.height
                       - 2 * antwika::music_editor::editorTheme()
                                 .buttonPadding;
    const auto rows = inner
                      / (antwika::gfx::kGlyphLineHeight
                         * antwika::music_editor::kTextScale);

    // The banded line sits one row above the bottom edge.
    std::string source;

    for (std::size_t line = 0; line + 2 < rows; ++line)
    {
        source += "// filler\n";
    }

    source += "$: drum.n(\"0\").pianoroll()\n";

    const auto frame = pianorollFrame(source);
    const auto band = frame.rects.find(pianorollBand(0));

    ASSERT_TRUE(band.has_value());
    EXPECT_LT(band->size.height, kRollHeight);
    EXPECT_TRUE(fillsIn(frame.commands, kRollBackdrop).empty());
}

namespace
{
    using antwika::music_editor::kWaveformRows;
    using antwika::music_editor::waveformBand;

    constexpr Color kWaveInk{
        .red = 110, .green = 170, .blue = 235, .alpha = 255};
} // namespace

// A waveform's band is a pianoroll's, base for base.
TEST(EditorSceneTest, AWaveformLineHoldsABandOfRoomUnderItself)
{
    const auto frame = pianorollFrame(
        "$: lead.n(\"0\").waveform()\n"
        "$: bass.n(\"0\")\n");

    const auto band = frame.rects.find(waveformBand(0));

    ASSERT_TRUE(band.has_value());
    EXPECT_EQ(
        band->size.height,
        kWaveformRows * antwika::gfx::kGlyphLineHeight
            * antwika::music_editor::kTextScale);

    const auto backdrops = fillsIn(frame.commands, kRollBackdrop);

    ASSERT_EQ(backdrops.size(), 1U);
    EXPECT_EQ(backdrops[0].rect, *band);
}

// A line asking for both stacks the roll over the wave.
TEST(EditorSceneTest, ARollAndAWaveStackUnderOneLine)
{
    const auto frame =
        pianorollFrame("$: drum.n(\"0\").pianoroll().waveform()\n");

    const auto roll = frame.rects.find(pianorollBand(0));
    const auto wave = frame.rects.find(waveformBand(0));

    ASSERT_TRUE(roll.has_value());
    ASSERT_TRUE(wave.has_value());
    EXPECT_EQ(
        wave->origin.y,
        roll->origin.y + static_cast<std::int32_t>(kRollHeight));
}

// A square at pitch nought shows exactly two oscillations.
// The first column sits on the midline's far side at full gain.
TEST(EditorSceneTest, PaintsASquareWaveOverItsBand)
{
    const auto frame = pianorollFrame(
        "$: n(\"0\").s(square).base(220).gain(1).waveform()\n");

    const auto band = frame.rects.find(waveformBand(0));

    ASSERT_TRUE(band.has_value());

    const auto columns = fillsIn(frame.commands, kWaveInk);

    // One column per four pixels, right across the band.
    ASSERT_EQ(columns.size(), (band->size.width + 3) / 4);

    const auto half =
        static_cast<std::int32_t>(band->size.height / 2);

    // Up first: the square's positive half, at full amplitude.
    EXPECT_EQ(
        columns[0].rect,
        (Rect{
            .origin = {.x = band->origin.x, .y = band->origin.y},
            .size =
                {.width = 4,
                 .height = static_cast<std::uint32_t>(half)}}));
}

// Each shape draws a wave of its own, and every pitch draws some.
// The octaves double and halve the oscillations, saturated far in.
TEST(EditorSceneTest, EveryShapeDrawsItsOwnWave)
{
    for (const auto *shape :
         {"sine", "saw", "square", "triangle", "noise"})
    {
        const auto frame = pianorollFrame(
            "$: n(\"0 84 -84\").s(" + std::string(shape)
            + ").base(220).gain(.5).waveform()\n");

        EXPECT_FALSE(fillsIn(frame.commands, kWaveInk).empty())
            << shape;
    }
}

TEST(EditorSceneTest, AWaveOfRestsIsItsBackdropAlone)
{
    const auto frame =
        pianorollFrame("$: drum.n(\"~ ~\").waveform()\n");

    EXPECT_EQ(fillsIn(frame.commands, kRollBackdrop).size(), 1U);
    EXPECT_TRUE(fillsIn(frame.commands, kWaveInk).empty());
}

// A note thinner than a pixel has no column to hold.
// So a thousand and twenty-four notes cross a three-hundred-pixel window.
// What is left is one one-pixel column per pixel that got a note at all.
TEST(EditorSceneTest, NotesThinnerThanAPixelHoldNoColumn)
{
    EditorState state;
    state.source = "$: drum.n(\"0*1024\").waveform()\n";

    Score score;
    score.read(state.source);

    const EditorScene scene;

    const auto frame = scene.describe(
        state,
        score,
        PlaybackStatus{},
        antwika::gfx::Size{.width = 300, .height = 640},
        Pointer{},
        Keyboard{});

    const auto band = frame.rects.find(waveformBand(0));

    ASSERT_TRUE(band.has_value());
    EXPECT_EQ(
        fillsIn(frame.commands, kWaveInk).size(), band->size.width);
}

TEST(EditorSceneTest, AWaveScrolledOffTheTopPaintsNothing)
{
    std::string source = "$: drum.n(\"0\").waveform()\n";

    for (std::size_t line = 0; line < 40; ++line)
    {
        source += "// filler\n";
    }

    const auto frame = pianorollFrame(source, 3);

    EXPECT_FALSE(frame.rects.find(waveformBand(0)).has_value());
    EXPECT_TRUE(fillsIn(frame.commands, kRollBackdrop).empty());
}

TEST(EditorSceneTest, AWaveCutShortByThePanesBottomPaintsNothing)
{
    const auto sized = pianorollFrame("$: drum.n(\"0\")\n");
    const auto pane = sized.rects.find(kCodeField);

    ASSERT_TRUE(pane.has_value());

    const auto inner = pane->size.height
                       - 2 * antwika::music_editor::editorTheme()
                                 .buttonPadding;
    const auto rows = inner
                      / (antwika::gfx::kGlyphLineHeight
                         * antwika::music_editor::kTextScale);

    std::string source;

    for (std::size_t line = 0; line + 2 < rows; ++line)
    {
        source += "// filler\n";
    }

    source += "$: drum.n(\"0\").waveform()\n";

    const auto frame = pianorollFrame(source);
    const auto band = frame.rects.find(waveformBand(0));

    ASSERT_TRUE(band.has_value());
    EXPECT_TRUE(fillsIn(frame.commands, kRollBackdrop).empty());
}
