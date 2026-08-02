#include "antwika/music_editor/Score.hpp"

#include <cstddef>
#include <string>
#include <string_view>

#include <gtest/gtest.h>

#include <antwika/pattern/Cycle.hpp>
#include <antwika/pattern/Span.hpp>

#include "antwika/music_editor/TrackPreset.hpp"

using antwika::music_editor::kTrackCount;
using antwika::music_editor::openingSource;
using antwika::music_editor::Problem;
using antwika::music_editor::Score;
using antwika::music_editor::trackFor;
using antwika::music_editor::trackName;
using antwika::pattern::Cycle;
using antwika::pattern::Span;

namespace
{
    const Span kFirstCycle(Cycle(), Cycle(1));

    [[nodiscard]] std::size_t eventsOn(
        const Score &score, std::size_t track)
    {
        return score.playing(track).queryAll(kFirstCycle).size();
    }

    // Asked for by name rather than written as an index.
    // So a test says what the document it reads says.
    [[nodiscard]] std::size_t named(std::string_view voice)
    {
        return trackFor(voice).value();
    }
} // namespace

TEST(ScoreTest, StartsSilentAndWithoutComplaint)
{
    const Score score;

    for (std::size_t track = 0; track < kTrackCount; ++track)
    {
        EXPECT_EQ(eventsOn(score, track), 0U) << track;
    }

    EXPECT_TRUE(score.problems().empty());
    EXPECT_FALSE(score.hasError());
    EXPECT_EQ(score.reparses(), 0U);
}

// An editor refusing its own opening document is one nobody trusts.
TEST(ScoreTest, TheOpeningDocumentParsesWithNoProblemsAtAll)
{
    Score score;

    score.read(openingSource());

    EXPECT_TRUE(score.problems().empty());
    EXPECT_FALSE(score.hasError());
    EXPECT_EQ(score.reparses(), kTrackCount);

    for (std::size_t track = 0; track < kTrackCount; ++track)
    {
        EXPECT_GT(eventsOn(score, track), 0U) << track;
    }
}

TEST(ScoreTest, ReadsAVoiceLineIntoWhatThatVoicePlays)
{
    Score score;

    score.read("$: bass 0 3 5\n");

    EXPECT_EQ(eventsOn(score, named("bass")), 3U);
    EXPECT_TRUE(score.problems().empty());
}

TEST(ScoreTest, PassesOverACommentAndABlankLine)
{
    Score score;

    score.read("// a comment\n\n   \n\t\n$: lead 0 3\n");

    EXPECT_TRUE(score.problems().empty());
    EXPECT_EQ(eventsOn(score, named("lead")), 2U);
}

// A voice is named rather than counted.
// So writing a line above another leaves that other one where it was.
TEST(ScoreTest, AVoiceKeepsItsInstrumentWhenALineIsWrittenAboveIt)
{
    Score score;

    score.read("$: bell 0 3\n");
    const auto before = eventsOn(score, named("bell"));

    score.read("$: bass 0\n$: bell 0 3\n");

    EXPECT_EQ(eventsOn(score, named("bell")), before);
    EXPECT_EQ(eventsOn(score, named("bass")), 1U);
    EXPECT_TRUE(score.problems().empty());
}

TEST(ScoreTest, RefusesALineThatDoesNotOpenWithTheVoiceMark)
{
    Score score;

    score.read("bass 0 3\n");

    ASSERT_EQ(score.problems().size(), 1U);
    EXPECT_EQ(score.problems()[0].line, 1U);
    EXPECT_FALSE(score.problems()[0].message.empty());
    EXPECT_TRUE(score.hasError());
}

TEST(ScoreTest, RefusesALineNamingNoVoiceItHas)
{
    Score score;

    score.read("$: horn 0 3\n");

    ASSERT_EQ(score.problems().size(), 1U);
    EXPECT_EQ(score.problems()[0].line, 1U);
}

// A voice mark with nothing after it names no voice either.
TEST(ScoreTest, RefusesAVoiceMarkOnItsOwn)
{
    Score score;

    score.read("$:\n");

    ASSERT_EQ(score.problems().size(), 1U);
    EXPECT_EQ(score.problems()[0].line, 1U);
}

// One voice, one line.
// A second is refused rather than taking the instrument off the first.
TEST(ScoreTest, RefusesASecondLineClaimingAVoiceAlreadySounding)
{
    Score score;

    score.read("$: bass 0\n$: bass 0 3 5\n");

    ASSERT_EQ(score.problems().size(), 1U);
    EXPECT_EQ(score.problems()[0].line, 2U);
    EXPECT_EQ(eventsOn(score, named("bass")), 1U);
}

TEST(ScoreTest, AVoiceLineWithNoNotationIsSilentRatherThanRefused)
{
    Score score;

    score.read("$: bass 0 3\n");
    ASSERT_EQ(eventsOn(score, named("bass")), 2U);

    score.read("$: bass\n");

    EXPECT_EQ(eventsOn(score, named("bass")), 0U);
    EXPECT_TRUE(score.problems().empty());
}

// The decision the whole feel of the editor rests on.
// Half a bracket is typed on the way to a whole one.
TEST(ScoreTest, ARefusedLineKeepsPlayingWhatItLastDid)
{
    Score score;

    score.read("$: bass 0 3 5\n");
    ASSERT_EQ(eventsOn(score, named("bass")), 3U);

    score.read("$: bass 0 3 5 [\n");

    EXPECT_EQ(eventsOn(score, named("bass")), 3U);
    ASSERT_EQ(score.problems().size(), 1U);
    EXPECT_EQ(score.problems()[0].line, 1U);
    EXPECT_TRUE(score.hasError());
}

// A line may parse cleanly and ask for something impossible.
// The algebra's refusal reads the same here as the grammar's.
TEST(ScoreTest, TheAlgebrasRefusalIsReportedToo)
{
    Score score;

    score.read("$: lead 0(9,8)\n");

    ASSERT_EQ(score.problems().size(), 1U);
    EXPECT_EQ(score.problems()[0].line, 1U);
    EXPECT_EQ(eventsOn(score, named("lead")), 0U);
}

TEST(ScoreTest, TheNewPatternTakesOverTheMomentItReads)
{
    Score score;

    score.read("$: bass 0 3 5 [\n");
    ASSERT_TRUE(score.hasError());

    score.read("$: bass 0 3 5 [7 9]\n");

    EXPECT_EQ(eventsOn(score, named("bass")), 5U);
    EXPECT_FALSE(score.hasError());
}

// Deleting a line is how an instrument is taken out.
TEST(ScoreTest, AVoiceNoLineNamesFallsSilent)
{
    Score score;

    score.read("$: bass 0\n$: drum 0 0\n");
    ASSERT_EQ(eventsOn(score, named("drum")), 2U);

    score.read("$: bass 0\n");

    EXPECT_EQ(eventsOn(score, named("drum")), 0U);
    EXPECT_EQ(eventsOn(score, named("bass")), 1U);
    EXPECT_TRUE(score.problems().empty());
}

// Its text is forgotten with it.
// So writing the same line again is heard rather than passed over.
TEST(ScoreTest, AVoiceWrittenAgainAfterBeingDeletedIsHeardAgain)
{
    Score score;

    score.read("$: drum 0 0\n");
    score.read("\n");
    ASSERT_EQ(eventsOn(score, named("drum")), 0U);

    score.read("$: drum 0 0\n");

    EXPECT_EQ(eventsOn(score, named("drum")), 2U);
}

// A refused line that is deleted takes its refusal with it.
TEST(ScoreTest, AVoiceDeletedWhileRefusedStopsBeingComplainedAbout)
{
    Score score;

    score.read("$: bass 0 [\n");
    ASSERT_TRUE(score.hasError());

    score.read("\n");

    EXPECT_FALSE(score.hasError());

    score.read("$: bass 0 [\n");

    EXPECT_TRUE(score.hasError());
}

// Cheap on a tick where nothing was typed, which is nearly every tick.
TEST(ScoreTest, ALineWhoseNotationDidNotChangeIsNotReadAgain)
{
    Score score;

    score.read("$: bass 0 3\n");
    const auto first = score.reparses();
    ASSERT_EQ(first, 1U);

    // The document changed and that line did not.
    score.read("$: bass 0 3\n// and a comment\n");

    EXPECT_EQ(score.reparses(), first);

    score.read("$: bass 0 3 5\n// and a comment\n");

    EXPECT_EQ(score.reparses(), first + 1);
}

TEST(ScoreTest, ADocumentThatDidNotChangeCostsNothing)
{
    Score score;

    score.read("$: bass 0 3\n");
    const auto first = score.reparses();

    score.read("$: bass 0 3\n");

    EXPECT_EQ(score.reparses(), first);
}

// The line is still refused, so it is still reported.
// That holds on a read which did not parse it again.
TEST(ScoreTest, ARefusalStandsForAsLongAsTheLineDoes)
{
    Score score;

    score.read("$: bass 0 [\n");
    ASSERT_EQ(score.problems().size(), 1U);
    const auto parsed = score.reparses();

    score.read("$: bass 0 [\n$: lead 0\n");

    ASSERT_EQ(score.problems().size(), 1U);
    EXPECT_EQ(score.problems()[0].line, 1U);
    EXPECT_EQ(score.reparses(), parsed + 1);
}

// A same-length change compares differently from one that grows.
// Both are worth reading again.
TEST(ScoreTest, ReadsALineAgainWhateverWayItChanged)
{
    Score score;

    score.read("$: bass 0 3\n");
    const auto first = score.reparses();

    score.read("$: bass 0 5\n");
    EXPECT_EQ(score.reparses(), first + 1);

    score.read("$: bass 0 5 7\n");
    EXPECT_EQ(score.reparses(), first + 2);

    score.read("$: bass\n");
    EXPECT_EQ(score.reparses(), first + 3);
}

TEST(ScoreTest, CountsLinesFromOne)
{
    Score score;

    score.read("// a comment\nnot a voice line\n$: horn 0\n");

    ASSERT_EQ(score.problems().size(), 2U);
    EXPECT_EQ(score.problems()[0].line, 2U);
    EXPECT_EQ(score.problems()[1].line, 3U);
}

// A document being typed has no trailing break until one is typed.
TEST(ScoreTest, ReadsALastLineWithNoBreakAfterIt)
{
    Score score;

    score.read("$: bass 0 3 5");

    EXPECT_EQ(eventsOn(score, named("bass")), 3U);
    EXPECT_TRUE(score.problems().empty());
}

TEST(ScoreTest, ReadsAnEmptyDocumentAsNothingAtAll)
{
    Score score;

    score.read("$: bass 0\n");
    ASSERT_EQ(eventsOn(score, named("bass")), 1U);

    score.read("");

    EXPECT_EQ(eventsOn(score, named("bass")), 0U);
    EXPECT_TRUE(score.problems().empty());
}

// Blanks either side of a line, and a tab between its words.
// That is what an indented document is written with.
TEST(ScoreTest, TrimsTheBlanksAroundAndInsideALine)
{
    Score score;

    score.read("  \t$: bell\t0 3  \t\n");

    EXPECT_EQ(eventsOn(score, named("bell")), 2U);
    EXPECT_TRUE(score.problems().empty());
}

TEST(ScoreTest, AProblemComparesFieldByField)
{
    const Problem problem{.line = 2, .message = "no"};

    EXPECT_EQ(problem, (Problem{.line = 2, .message = "no"}));
    EXPECT_NE(problem, (Problem{.line = 3, .message = "no"}));
    EXPECT_NE(problem, (Problem{.line = 2, .message = "yes"}));
}

// Written in the syntax the editor is for, so the editor reads it.
TEST(ScoreTest, TheOpeningDocumentNamesEveryVoiceOnce)
{
    const auto source = openingSource();

    EXPECT_NE(source.find("//"), std::string::npos);

    for (std::size_t track = 0; track < kTrackCount; ++track)
    {
        const std::string line = "$: " + std::string(trackName(track));

        EXPECT_NE(source.find(line), std::string::npos) << track;
    }
}
