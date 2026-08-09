#include <gtest/gtest.h>

#include <algorithm>
#include <cstddef>
#include <string>
#include <variant>
#include <vector>

#include <antwika/gfx/Glyphs.hpp>
#include <antwika/gfx/Rect.hpp>
#include <antwika/ui/DrawCommand.hpp>
#include <antwika/ui/Frame.hpp>
#include <antwika/ui/Keyboard.hpp>
#include <antwika/ui/Pointer.hpp>

#include "antwika/music_editor/EditorScene.hpp"
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

    [[nodiscard]] EditorState refusedLines(std::size_t count)
    {
        EditorState state;

        for (std::size_t line = 0; line < count; ++line)
        {
            state.source += "no\n";
        }

        return state;
    }
}

TEST(EditorSceneTest, Describe_DrawsItsTextAtTwiceTheGlyphScale)
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

TEST(EditorSceneTest, Describe_DrawsOneCodePaneAndBothButtons)
{
    const auto state = openingState();
    const Score score;

    const auto frame = describe(state, score);

    EXPECT_FALSE(frame.commands.empty());
    EXPECT_TRUE(frame.rects.find(kCodeField).has_value());
    EXPECT_TRUE(frame.rects.find(kPlayButton).has_value());
    EXPECT_TRUE(frame.rects.find(kPanicButton).has_value());
}

TEST(EditorSceneTest, Describe_DrawsEveryLineOfTheDocument)
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
TEST(EditorSceneTest, Describe_SaysWhetherItIsPlayingOrPaused)
{
    auto state = openingState();
    const Score score;

    const auto playing = describe(state, score).commands;
    EXPECT_TRUE(says(playing, "playing"));

    state.paused = true;
    const auto paused = describe(state, score).commands;

    EXPECT_TRUE(says(paused, "paused"));
    EXPECT_NE(playing, paused);

    EXPECT_TRUE(says(playing, "pause"));
    EXPECT_TRUE(says(paused, "resume"));
}

TEST(EditorSceneTest, Describe_SaysNothingWhenNothingRefused)
{
    const auto state = openingState();
    Score score;
    score.read(state.source);
    ASSERT_FALSE(score.hasError());

    EXPECT_FALSE(says(describe(state, score).commands, "line "));
}

TEST(EditorSceneTest, Describe_NamesTheLineThatWasRefused)
{
    EditorState state;
    state.source = "$: bass.n(\"0\")\nnot a voice line\n";

    Score score;
    score.read(state.source);
    ASSERT_EQ(score.problems().size(), 1U);

    EXPECT_TRUE(says(describe(state, score).commands, "line 2: "));
}

TEST(EditorSceneTest, Describe_ShowsThreeProblemsAndCounts)
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

TEST(EditorSceneTest, Describe_SaysWhatIsPlayingWithoutReaching)
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

TEST(EditorSceneTest, Describe_ShowsAnExampleWhileThereIsNothingToShow)
{
    EditorState state;
    const Score score;

    EXPECT_TRUE(says(describe(state, score).commands, "$: bass"));
}

TEST(EditorSceneTest, Describe_MovingTheCaretChangesThePicture)
{
    auto state = openingState();
    const Score score;

    const auto atEnd = describe(state, score).commands;

    state.cursor = 0;

    EXPECT_NE(describe(state, score).commands, atEnd);
}

TEST(EditorSceneTest, Describe_TheMenuIsAtTheTopAndClosedByDefault)
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

TEST(EditorSceneTest, Describe_AnOpenMenuListsItsFourCommands)
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

TEST(EditorSceneTest, DescribeModal_AsksANameAndOffersButtons)
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

TEST(EditorSceneTest, DescribeModal_TheLoadBoxListsEveryScoreAsAButton)
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

    EXPECT_FALSE(
        frame.rects.find(antwika::music_editor::kSaveConfirm)
            .has_value());
}

TEST(EditorSceneTest, DescribeModal_SaysWhenThereIsNothingToLoad)
{
    const EditorScene scene;

    EditorState state = openingState();
    state.modal = antwika::music_editor::Modal::Load;

    const auto frame = scene.describeModal(
        state, kCanvas, Pointer{}, Keyboard{});

    EXPECT_TRUE(says(frame.commands, "nothing saved yet"));
}

TEST(EditorSceneTest, DescribeModal_ABoxShowsItsNoticeInItsOwnInk)
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

    constexpr Color kRollBackdrop{
        .red = 12, .green = 13, .blue = 18, .alpha = 255};

    constexpr Color kNoteInk{
        .red = 130, .green = 205, .blue = 140, .alpha = 255};

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
}

TEST(EditorSceneTest, Describe_ARollOfRestsIsItsBackdropAlone)
{
    const auto frame =
        pianorollFrame("$: drum.n(\"~ ~\").pianoroll()\n");

    EXPECT_EQ(fillsIn(frame.commands, kRollBackdrop).size(), 1U);
    EXPECT_TRUE(fillsIn(frame.commands, kNoteInk).empty());
}

TEST(EditorSceneTest, Describe_EmptiesARollWhosePatternRefuses)
{
    const auto frame = pianorollFrame(
        "$: bass.n(\"0/1000/1000/1000/1000/1000/1000/1000\")"
        ".pianoroll()\n");

    EXPECT_EQ(fillsIn(frame.commands, kRollBackdrop).size(), 1U);
    EXPECT_TRUE(fillsIn(frame.commands, kNoteInk).empty());
}

TEST(EditorSceneTest, Describe_ARollScrolledOffTheTopPaintsNothing)
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

TEST(EditorSceneTest, Describe_PaintsNoRollCutByTheBottom)
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
}

TEST(EditorSceneTest, Describe_HoldsABandUnderAWaveformLine)
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

TEST(EditorSceneTest, Describe_ARollAndAWaveStackUnderOneLine)
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

namespace
{
    using antwika::music_editor::WaveImage;

    [[nodiscard]] antwika::ui::Frame waveFrame(
        const std::string &source,
        const std::vector<WaveImage> &waves)
    {
        EditorState state;
        state.source = source;

        Score score;
        score.read(state.source);

        return describe(
            state, score, PlaybackStatus{.waves = waves});
    }
}

TEST(EditorSceneTest, Describe_PaintsTheRenderedWaveOverItsBand)
{
    const std::vector<WaveImage> waves{WaveImage{
        .low = {-1.0F, 0.0F, 0.0F, -0.5F},
        .high = {1.0F, 0.5F, 0.0F, 0.0F}}};

    const auto frame =
        waveFrame("$: drum.n(\"0\").waveform()\n", waves);

    const auto band = frame.rects.find(waveformBand(0));

    ASSERT_TRUE(band.has_value());

    const auto columns = fillsIn(frame.commands, kWaveInk);

    ASSERT_EQ(columns.size(), band->size.width);

    const auto half =
        static_cast<std::int32_t>(band->size.height / 2);

    EXPECT_EQ(
        columns[0].rect,
        (Rect{
            .origin = {.x = band->origin.x, .y = band->origin.y},
            .size =
                {.width = 1,
                 .height = static_cast<std::uint32_t>(2 * half)}}));

    const auto tail = columns[(band->size.width / 4) * 3].rect;

    EXPECT_EQ(tail.origin.y, band->origin.y + half);
    EXPECT_EQ(
        tail.size.height, static_cast<std::uint32_t>(half / 2));
}

TEST(EditorSceneTest, Describe_ASilentImageDrawsItsMidline)
{
    const std::vector<WaveImage> waves{WaveImage{
        .low = {0.0F, 0.0F}, .high = {0.0F, 0.0F}}};

    const auto frame =
        waveFrame("$: drum.n(\"~ ~\").waveform()\n", waves);

    const auto band = frame.rects.find(waveformBand(0));

    ASSERT_TRUE(band.has_value());

    const auto columns = fillsIn(frame.commands, kWaveInk);

    ASSERT_EQ(columns.size(), band->size.width);
    EXPECT_EQ(columns[0].rect.size.height, 1U);
    EXPECT_EQ(
        columns[0].rect.origin.y,
        band->origin.y
            + static_cast<std::int32_t>(band->size.height / 2));
}

TEST(EditorSceneTest, Describe_AWaveWithNoImageIsItsBackdropAlone)
{
    const auto frame =
        pianorollFrame("$: drum.n(\"~ ~\").waveform()\n");

    EXPECT_EQ(fillsIn(frame.commands, kRollBackdrop).size(), 1U);
    EXPECT_TRUE(fillsIn(frame.commands, kWaveInk).empty());
}

TEST(EditorSceneTest, Describe_AnEmptyImageIsItsBackdropAlone)
{
    const auto frame = waveFrame(
        "$: drum.n(\"~ ~\").waveform()\n",
        std::vector<WaveImage>{WaveImage{}});

    EXPECT_EQ(fillsIn(frame.commands, kRollBackdrop).size(), 1U);
    EXPECT_TRUE(fillsIn(frame.commands, kWaveInk).empty());
}

TEST(EditorSceneTest, Describe_AWaveScrolledOffTheTopPaintsNothing)
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

TEST(EditorSceneTest, Describe_PaintsNoWaveCutByTheBottom)
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

namespace
{
    [[nodiscard]] antwika::ui::Frame pacedFrame(
        const std::string &source)
    {
        EditorState state;
        state.source = source;

        Score score;
        score.read(state.source);

        return describe(
            state,
            score,
            PlaybackStatus{.rate = 48000, .cycleFrames = 48000});
    }
}

namespace
{
    using antwika::sequencer::Rational;

    [[nodiscard]] antwika::ui::Frame rolledFrame(
        const std::string &source,
        const Rational position,
        const PlaybackStatus &extra = PlaybackStatus{})
    {
        EditorState state;
        state.source = source;

        Score score;
        score.read(state.source);

        auto status = extra;
        status.position = position;

        return describe(state, score, status);
    }
}

namespace
{
    constexpr Color kPlayingInk{
        .red = 235, .green = 245, .blue = 160, .alpha = 255};

    constexpr Color kNowInk{
        .red = 225, .green = 230, .blue = 235, .alpha = 255};

    [[nodiscard]] std::uint32_t rollWidthOf(
        const antwika::gfx::Rect &band)
    {
        return std::min(
            band.size.width, antwika::music_editor::kPianorollWidth);
    }
}

TEST(EditorSceneTest, Describe_HoldsABandUnderAPianorollLine)
{
    const auto frame = pianorollFrame(
        "$: drum.n(\"0 3\").pianoroll()\n"
        "$: bass.n(\"0\")\n");

    const auto band = frame.rects.find(pianorollBand(0));

    ASSERT_TRUE(band.has_value());
    EXPECT_EQ(band->size.height, kRollHeight);
    EXPECT_LT(rollWidthOf(*band), band->size.width);

    const auto backdrops = fillsIn(frame.commands, kRollBackdrop);

    ASSERT_EQ(backdrops.size(), 1U);
    EXPECT_EQ(
        backdrops[0].rect,
        (Rect{
            .origin = band->origin,
            .size = {
                .width = rollWidthOf(*band),
                .height = kRollHeight}}));
}

TEST(EditorSceneTest, Describe_TheRollDrawsItsPlayheadLine)
{
    const auto frame =
        pianorollFrame("$: drum.n(\"0 3\").pianoroll()\n");

    const auto band = frame.rects.find(pianorollBand(0));

    ASSERT_TRUE(band.has_value());

    const auto lines = fillsIn(frame.commands, kNowInk);

    ASSERT_EQ(lines.size(), 1U);
    EXPECT_EQ(
        lines[0].rect,
        (Rect{
            .origin =
                {.x = band->origin.x
                      + static_cast<std::int32_t>(
                          rollWidthOf(*band) / 4),
                 .y = band->origin.y},
            .size = {.width = 2, .height = kRollHeight}}));
}

TEST(EditorSceneTest, Describe_PaintsTheRollsNotesOverItsBand)
{
    const auto frame =
        pianorollFrame("$: drum.n(\"0 3\").pianoroll()\n");

    const auto band = frame.rects.find(pianorollBand(0));

    ASSERT_TRUE(band.has_value());

    const auto width = static_cast<std::int64_t>(rollWidthOf(*band));
    const auto lane = kRollHeight / 4;

    const auto playing = fillsIn(frame.commands, kPlayingInk);

    ASSERT_EQ(playing.size(), 1U);
    EXPECT_EQ(
        playing[0].rect,
        (Rect{
            .origin =
                {.x = band->origin.x
                      + static_cast<std::int32_t>(width / 4),
                 .y = band->origin.y
                      + static_cast<std::int32_t>(kRollHeight - lane)},
            .size =
                {.width = static_cast<std::uint32_t>(width / 2),
                 .height = lane}}));

    const auto cells = fillsIn(frame.commands, kNoteInk);

    ASSERT_EQ(cells.size(), 2U);
    EXPECT_EQ(cells[0].rect.origin.x, band->origin.x);
    EXPECT_EQ(cells[0].rect.origin.y, band->origin.y);
    EXPECT_EQ(
        cells[0].rect.size.width,
        static_cast<std::uint32_t>(width / 4));
    EXPECT_EQ(
        cells[1].rect.origin.x,
        band->origin.x + static_cast<std::int32_t>(3 * width / 4));
}

TEST(EditorSceneTest, Describe_DrawsNoLanePastTheBandsTop)
{
    const auto frame =
        pianorollFrame("$: drum.n(\"0 50\").pianoroll()\n");

    const auto playing = fillsIn(frame.commands, kPlayingInk);
    const auto cells = fillsIn(frame.commands, kNoteInk);

    ASSERT_EQ(playing.size(), 1U);
    EXPECT_TRUE(cells.empty());
    EXPECT_EQ(playing[0].rect.size.height, 1U);
}

TEST(EditorSceneTest, Describe_SizesARollCellToItsNote)
{
    const auto frame = pacedFrame(
        "$: drum.n(\"0 3\").hold(100).rel(25).pianoroll()\n");

    const auto band = frame.rects.find(pianorollBand(0));

    ASSERT_TRUE(band.has_value());

    const auto width = static_cast<std::int64_t>(rollWidthOf(*band));
    const auto sounding =
        static_cast<std::uint32_t>(6000 * width / 48000);

    const auto playing = fillsIn(frame.commands, kPlayingInk);
    const auto cells = fillsIn(frame.commands, kNoteInk);

    ASSERT_EQ(playing.size(), 1U);
    ASSERT_EQ(cells.size(), 1U);

    EXPECT_EQ(playing[0].rect.size.width, sounding);
    EXPECT_EQ(cells[0].rect.size.width, sounding);
    EXPECT_EQ(
        cells[0].rect.origin.x,
        band->origin.x + static_cast<std::int32_t>(3 * width / 4));
}

TEST(EditorSceneTest, Describe_ShowsAReleaseRingingPastTheSlot)
{
    const auto frame = pacedFrame(
        "$: n(\"0 ~\").hold(2000).rel(1000).pianoroll()\n");

    const auto band = frame.rects.find(pianorollBand(0));

    ASSERT_TRUE(band.has_value());

    const auto width = static_cast<std::int64_t>(rollWidthOf(*band));

    const auto playing = fillsIn(frame.commands, kPlayingInk);

    ASSERT_EQ(playing.size(), 2U);

    EXPECT_EQ(playing[0].rect.origin.x, band->origin.x);
    EXPECT_EQ(
        playing[0].rect.size.width,
        static_cast<std::uint32_t>(3 * width / 4));
    EXPECT_EQ(
        playing[1].rect.origin.x,
        band->origin.x + static_cast<std::int32_t>(width / 4));
    EXPECT_EQ(
        playing[1].rect.size.width,
        static_cast<std::uint32_t>(width - width / 4));
}

TEST(EditorSceneTest, Describe_FillsTheSlotWithoutACycleLength)
{
    EditorState state;
    state.source =
        "$: drum.n(\"0 ~\").hold(100).rel(25).pianoroll()\n";

    Score score;
    score.read(state.source);

    const auto frame = describe(
        state,
        score,
        PlaybackStatus{.rate = 48000, .cycleFrames = 0});

    const auto band = frame.rects.find(pianorollBand(0));

    ASSERT_TRUE(band.has_value());

    const auto playing = fillsIn(frame.commands, kPlayingInk);

    ASSERT_EQ(playing.size(), 1U);
    EXPECT_EQ(
        playing[0].rect.size.width, rollWidthOf(*band) / 2);
    EXPECT_TRUE(fillsIn(frame.commands, kNoteInk).empty());
}

TEST(EditorSceneTest, Describe_TheRollScrollsWithThePlayhead)
{
    const auto frame = rolledFrame(
        "$: drum.n(\"0 3\").pianoroll()\n", Rational{1, 2});

    const auto band = frame.rects.find(pianorollBand(0));

    ASSERT_TRUE(band.has_value());

    const auto width = static_cast<std::int64_t>(rollWidthOf(*band));

    const auto playing = fillsIn(frame.commands, kPlayingInk);
    const auto cells = fillsIn(frame.commands, kNoteInk);

    ASSERT_EQ(playing.size(), 1U);
    ASSERT_EQ(cells.size(), 2U);

    EXPECT_EQ(
        playing[0].rect.origin.x,
        band->origin.x + static_cast<std::int32_t>(width / 4));
    EXPECT_EQ(playing[0].rect.origin.y, band->origin.y);

    EXPECT_EQ(cells[0].rect.origin.x, band->origin.x);
    EXPECT_EQ(
        cells[1].rect.origin.x,
        band->origin.x + static_cast<std::int32_t>(3 * width / 4));
}

TEST(EditorSceneTest, Describe_ShowsAnUpcomingAlternation)
{
    const auto frame = rolledFrame(
        "$: n(\"0 <3 5>\").pianoroll()\n", Rational{3, 2});

    const auto band = frame.rects.find(pianorollBand(0));

    ASSERT_TRUE(band.has_value());

    const auto playing = fillsIn(frame.commands, kPlayingInk);
    const auto cells = fillsIn(frame.commands, kNoteInk);

    ASSERT_EQ(playing.size(), 1U);
    ASSERT_EQ(cells.size(), 2U);

    EXPECT_EQ(playing[0].rect.origin.y, band->origin.y);
}

TEST(EditorSceneTest, Describe_ReachesARingingNoteInFromLeft)
{
    const auto frame = rolledFrame(
        "$: drum.n(\"0 _ 3\").pianoroll()\n", Rational{1, 2});

    const auto band = frame.rects.find(pianorollBand(0));

    ASSERT_TRUE(band.has_value());

    const auto width = static_cast<std::int64_t>(rollWidthOf(*band));

    const auto playing = fillsIn(frame.commands, kPlayingInk);

    ASSERT_EQ(playing.size(), 1U);
    EXPECT_EQ(playing[0].rect.origin.x, band->origin.x);
    EXPECT_EQ(
        playing[0].rect.size.width,
        static_cast<std::uint32_t>(5 * width / 12));

    EXPECT_EQ(fillsIn(frame.commands, kNoteInk).size(), 2U);
}

TEST(EditorSceneTest, Describe_DrawsNoCellForAnEndedTail)
{
    const auto frame = rolledFrame(
        "$: drum.n(\"0 ~\").hold(100).rel(25).pianoroll()\n",
        Rational{1, 2},
        PlaybackStatus{.rate = 48000, .cycleFrames = 48000});

    const auto band = frame.rects.find(pianorollBand(0));

    ASSERT_TRUE(band.has_value());

    const auto cells = fillsIn(frame.commands, kNoteInk);

    EXPECT_TRUE(fillsIn(frame.commands, kPlayingInk).empty());
    ASSERT_EQ(cells.size(), 1U);
    EXPECT_EQ(
        cells[0].rect.origin.x,
        band->origin.x
            + static_cast<std::int32_t>(
                3 * rollWidthOf(*band) / 4));
}

TEST(EditorSceneTest, Describe_ARollDrawsOneCellPerOnset)
{
    const auto frame =
        pianorollFrame("$: drum.n(\"0/2\").pianoroll()\n");

    const auto band = frame.rects.find(pianorollBand(0));

    ASSERT_TRUE(band.has_value());

    const auto width = static_cast<std::int64_t>(rollWidthOf(*band));

    const auto playing = fillsIn(frame.commands, kPlayingInk);

    ASSERT_EQ(playing.size(), 1U);
    EXPECT_TRUE(fillsIn(frame.commands, kNoteInk).empty());
    EXPECT_EQ(
        playing[0].rect.origin.x,
        band->origin.x + static_cast<std::int32_t>(width / 4));
}
