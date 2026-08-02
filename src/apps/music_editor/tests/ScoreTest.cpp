#include "antwika/music_editor/Score.hpp"

#include <cstddef>
#include <string>

#include <gtest/gtest.h>

#include <antwika/pattern/Cycle.hpp>
#include <antwika/pattern/Span.hpp>

#include "antwika/music_editor/TrackPreset.hpp"

using antwika::music_editor::kPresetCount;
using antwika::music_editor::openingSource;
using antwika::music_editor::Problem;
using antwika::music_editor::Score;
using antwika::music_editor::trackFor;
using antwika::music_editor::trackName;
using antwika::music_editor::TrackPreset;
using antwika::music_editor::trackPresets;
using antwika::music_editor::Voice;
using antwika::pattern::Cycle;
using antwika::pattern::Span;

namespace
{
    const Span kFirstCycle(Cycle(), Cycle(1));

    // How many events one voice's line puts in the first cycle.
    [[nodiscard]] std::size_t eventsOn(
        const Score &score, const std::size_t voice)
    {
        const auto &playing = score.voices()[voice].playing;

        return playing.queryAll(kFirstCycle).size();
    }

    [[nodiscard]] TrackPreset preset(const std::string &name)
    {
        return trackPresets()[trackFor(name).value()];
    }
} // namespace

TEST(ScoreTest, StartsSilentAndWithoutComplaint)
{
    const Score score;

    EXPECT_TRUE(score.voices().empty());
    EXPECT_TRUE(score.problems().empty());
    EXPECT_FALSE(score.hasError());
    EXPECT_EQ(score.reparses(), 0U);
}

// An editor refusing its own opening document is one nobody trusts.
TEST(ScoreTest, TheOpeningDocumentReadsWithNoProblemsAndFourVoices)
{
    Score score;

    score.read(openingSource());

    EXPECT_TRUE(score.problems().empty());
    EXPECT_FALSE(score.hasError());
    ASSERT_EQ(score.voices().size(), 4U);
    EXPECT_EQ(score.reparses(), 4U);

    for (std::size_t voice = 0; voice < score.voices().size(); ++voice)
    {
        EXPECT_GT(eventsOn(score, voice), 0U) << voice;
    }
}

// Written in the syntax the editor is for, so the editor reads it.
TEST(ScoreTest, TheOpeningDocumentShowsTheShapeOfTheLanguage)
{
    const auto source = openingSource();

    EXPECT_NE(source.find("//"), std::string::npos);
    EXPECT_NE(source.find("$: "), std::string::npos);
    EXPECT_NE(source.find(".n(\""), std::string::npos);

    // Two drums, because one of a kind is not the rule here.
    const auto first = source.find("$: drum.");
    ASSERT_NE(first, std::string::npos);
    EXPECT_NE(source.find("$: drum.", first + 1), std::string::npos);
}

TEST(ScoreTest, ReadsAVoiceLineIntoAVoice)
{
    Score score;

    score.read("$: bass.n(\"0 3 5\")\n");

    ASSERT_EQ(score.voices().size(), 1U);
    EXPECT_EQ(eventsOn(score, 0), 3U);
    EXPECT_EQ(score.voices()[0].preset, preset("bass"));
    EXPECT_TRUE(score.problems().empty());
}

TEST(ScoreTest, PassesOverACommentAndABlankLine)
{
    Score score;

    score.read("// a comment\n\n   \n\t\n$: lead.n(\"0 3\")\n");

    EXPECT_TRUE(score.problems().empty());
    ASSERT_EQ(score.voices().size(), 1U);
    EXPECT_EQ(eventsOn(score, 0), 2U);
}

// A comment ends a line wherever it starts, not only where it opens.
TEST(ScoreTest, ReadsAVoiceLineWithACommentAfterIt)
{
    Score score;

    score.read("$: drum.n(\"0(3,8)\")   // the kick\n");

    EXPECT_TRUE(score.problems().empty());
    ASSERT_EQ(score.voices().size(), 1U);
    EXPECT_EQ(eventsOn(score, 0), 3U);
    EXPECT_EQ(score.voices()[0].preset, preset("drum"));
}

// A whole-line comment is a line holding nothing, by the same rule.
TEST(ScoreTest, AWholeLineCommentIsCutAwayLikeAnyOther)
{
    Score score;

    score.read("   // nothing to see\n$: bass.n(\"0\") // nor here\n");

    EXPECT_TRUE(score.problems().empty());
    EXPECT_EQ(score.voices().size(), 1U);
}

// The notation is never cut into, whatever it holds.
TEST(ScoreTest, ACommentMarkInsideQuotesIsNotAComment)
{
    Score score;

    score.read("$: bass.n(\"0 // 3\")\n");

    // It reached the notation, which is what refuses it.
    ASSERT_EQ(score.problems().size(), 1U);
    EXPECT_EQ(score.problems()[0].line, 1U);
    EXPECT_FALSE(score.problems()[0].message.empty());
    EXPECT_TRUE(score.voices().empty());
}

// One slash is not two, even at the very end of a line.
TEST(ScoreTest, ALoneSlashIsNotACommentMark)
{
    Score score;

    score.read("/\n");

    ASSERT_EQ(score.problems().size(), 1U);
    EXPECT_EQ(score.problems()[0].line, 1U);
}

// A preset is a starting point, so two drum lines are two voices.
TEST(ScoreTest, TwoLinesOpeningWithOnePresetAreTwoVoices)
{
    Score score;

    score.read(
        "$: drum.n(\"0(3,8)\")\n"
        "$: drum.n(\"0 0\").gain(.12).pan(.5)\n");

    ASSERT_EQ(score.voices().size(), 2U);
    EXPECT_TRUE(score.problems().empty());

    EXPECT_EQ(eventsOn(score, 0), 3U);
    EXPECT_EQ(eventsOn(score, 1), 2U);

    // They sound together and differ in every other respect.
    EXPECT_NE(score.voices()[0].preset, score.voices()[1].preset);
    EXPECT_EQ(score.voices()[0].preset, preset("drum"));
    EXPECT_FLOAT_EQ(score.voices()[1].preset.gain, 0.12F);
    EXPECT_FLOAT_EQ(score.voices()[1].preset.pan, 0.5F);
}

// Each line is read on its own, from the preset it names and no other.
TEST(ScoreTest, WhatOneLineChangesDoesNotReachTheNext)
{
    Score score;

    score.read(
        "$: bass.n(\"0\").gain(.9).o(2)\n"
        "$: bass.n(\"0\")\n"
        "$: n(\"0\")\n");

    ASSERT_EQ(score.voices().size(), 3U);

    EXPECT_EQ(score.voices()[1].preset, preset("bass"));
    EXPECT_EQ(score.voices()[1].preset.transpose, 0);
    EXPECT_EQ(score.voices()[2].preset, TrackPreset{});
}

// A voice is a line, so deleting the line takes the voice out.
TEST(ScoreTest, ALineDeletedTakesItsVoiceWithIt)
{
    Score score;

    score.read("$: bass.n(\"0\")\n$: drum.n(\"0 0\")\n");
    ASSERT_EQ(score.voices().size(), 2U);

    score.read("$: bass.n(\"0\")\n");

    ASSERT_EQ(score.voices().size(), 1U);
    EXPECT_EQ(eventsOn(score, 0), 1U);
    EXPECT_TRUE(score.problems().empty());
}

// Its text is forgotten with it, so writing it again is heard again.
TEST(ScoreTest, ALineWrittenAgainAfterBeingDeletedIsHeardAgain)
{
    Score score;

    score.read("$: drum.n(\"0 0\")\n");
    score.read("\n");
    ASSERT_TRUE(score.voices().empty());

    score.read("$: drum.n(\"0 0\")\n");

    ASSERT_EQ(score.voices().size(), 1U);
    EXPECT_EQ(eventsOn(score, 0), 2U);
}

// The decision the whole feel of the editor rests on.
// Half a bracket is typed on the way to a whole one.
TEST(ScoreTest, ARefusedLineKeepsPlayingWhatItLastDid)
{
    Score score;

    score.read("$: bass.n(\"0 3 5\")\n");
    ASSERT_EQ(eventsOn(score, 0), 3U);

    score.read("$: bass.n(\"0 3 5 [\")\n");

    ASSERT_EQ(score.voices().size(), 1U);
    EXPECT_EQ(eventsOn(score, 0), 3U);
    ASSERT_EQ(score.problems().size(), 1U);
    EXPECT_EQ(score.problems()[0].line, 1U);
    EXPECT_TRUE(score.hasError());
}

// A line that has never read has nothing to keep sounding.
TEST(ScoreTest, ALineRefusedBeforeItEverReadContributesNoVoice)
{
    Score score;

    score.read("$: horn.n(\"0\")\n$: bass.n(\"0 3\")\n");

    ASSERT_EQ(score.problems().size(), 1U);
    EXPECT_EQ(score.problems()[0].line, 1U);

    // Only the line that read is sounding.
    ASSERT_EQ(score.voices().size(), 1U);
    EXPECT_EQ(eventsOn(score, 0), 2U);
    EXPECT_EQ(score.voices()[0].preset, preset("bass"));
}

// Each of the three refusals arrives at the same place.
TEST(ScoreTest, ReportsWhicheverOfTheThreeRefusalsArrives)
{
    Score chain;
    chain.read("$: n(\"0\").wobble(1)\n");
    EXPECT_EQ(chain.problems().size(), 1U);

    Score grammar;
    grammar.read("$: n(\"0 [\")\n");
    EXPECT_EQ(grammar.problems().size(), 1U);

    // It read cleanly and asked for something impossible.
    Score algebra;
    algebra.read("$: n(\"0(9,8)\")\n");
    EXPECT_EQ(algebra.problems().size(), 1U);
}

TEST(ScoreTest, RefusesALineThatDoesNotOpenWithTheVoiceMark)
{
    Score score;

    score.read("bass.n(\"0 3\")\n");

    ASSERT_EQ(score.problems().size(), 1U);
    EXPECT_EQ(score.problems()[0].line, 1U);
    EXPECT_FALSE(score.problems()[0].message.empty());
    EXPECT_TRUE(score.hasError());
    EXPECT_TRUE(score.voices().empty());
}

// A voice mark with nothing after it carries no chain either.
TEST(ScoreTest, RefusesALineEmptiedDownToItsVoiceMark)
{
    Score score;

    score.read("$: bass.n(\"0\")\n");
    ASSERT_TRUE(score.problems().empty());

    score.read("$:\n");

    ASSERT_EQ(score.problems().size(), 1U);
    EXPECT_EQ(score.problems()[0].line, 1U);

    // And it keeps playing whatever it last did while it is refused.
    ASSERT_EQ(score.voices().size(), 1U);
    EXPECT_EQ(eventsOn(score, 0), 1U);
}

TEST(ScoreTest, TheNewChainTakesOverTheMomentItReads)
{
    Score score;

    score.read("$: bass.n(\"0 3 [\")\n");
    ASSERT_TRUE(score.hasError());

    score.read("$: bass.n(\"0 3 [5 7]\")\n");

    EXPECT_FALSE(score.hasError());
    ASSERT_EQ(score.voices().size(), 1U);
    EXPECT_EQ(eventsOn(score, 0), 4U);
}

// A refused line that is deleted takes its refusal with it.
TEST(ScoreTest, ALineDeletedWhileRefusedStopsBeingComplainedAbout)
{
    Score score;

    score.read("$: bass.n(\"0 [\")\n");
    ASSERT_TRUE(score.hasError());

    score.read("\n");
    EXPECT_FALSE(score.hasError());

    score.read("$: bass.n(\"0 [\")\n");
    EXPECT_TRUE(score.hasError());
}

// The line is still refused, so it is still reported.
// That holds on a read which did not parse it again.
TEST(ScoreTest, ARefusalStandsForAsLongAsTheLineDoes)
{
    Score score;

    score.read("$: bass.n(\"0 [\")\n");
    ASSERT_EQ(score.problems().size(), 1U);
    const auto parsed = score.reparses();

    score.read("$: bass.n(\"0 [\")\n$: lead.n(\"0\")\n");

    ASSERT_EQ(score.problems().size(), 1U);
    EXPECT_EQ(score.problems()[0].line, 1U);
    EXPECT_EQ(score.reparses(), parsed + 1);
}

// Cheap on a tick where nothing was typed, which is nearly every tick.
TEST(ScoreTest, ALineWhoseTextDidNotChangeIsNotReadAgain)
{
    Score score;

    score.read("$: bass.n(\"0 3\")\n");
    const auto first = score.reparses();
    ASSERT_EQ(first, 1U);

    // The document changed and that line did not.
    score.read("$: bass.n(\"0 3\")\n// and a comment\n");
    EXPECT_EQ(score.reparses(), first);

    score.read("$: bass.n(\"0 3 5\")\n// and a comment\n");
    EXPECT_EQ(score.reparses(), first + 1);
}

TEST(ScoreTest, ADocumentThatDidNotChangeCostsNothing)
{
    Score score;

    score.read("$: bass.n(\"0 3\")\n");
    const auto first = score.reparses();

    score.read("$: bass.n(\"0 3\")\n");

    EXPECT_EQ(score.reparses(), first);
}

// A same-length change compares differently from one that grows.
TEST(ScoreTest, ReadsALineAgainWhateverWayItChanged)
{
    Score score;

    score.read("$: bass.n(\"0 3\")\n");
    const auto first = score.reparses();

    score.read("$: bass.n(\"0 5\")\n");
    EXPECT_EQ(score.reparses(), first + 1);

    score.read("$: bass.n(\"0 5 7\")\n");
    EXPECT_EQ(score.reparses(), first + 2);

    score.read("$: lead.n(\"0 5 7\")\n");
    EXPECT_EQ(score.reparses(), first + 3);
}

TEST(ScoreTest, CountsLinesFromOne)
{
    Score score;

    score.read("// a comment\nnot a voice line\n$: horn.n(\"0\")\n");

    ASSERT_EQ(score.problems().size(), 2U);
    EXPECT_EQ(score.problems()[0].line, 2U);
    EXPECT_EQ(score.problems()[1].line, 3U);
}

// A document being typed has no trailing break until one is typed.
TEST(ScoreTest, ReadsALastLineWithNoBreakAfterIt)
{
    Score score;

    score.read("$: bass.n(\"0 3 5\")");

    ASSERT_EQ(score.voices().size(), 1U);
    EXPECT_EQ(eventsOn(score, 0), 3U);
    EXPECT_TRUE(score.problems().empty());
}

TEST(ScoreTest, ReadsAnEmptyDocumentAsNothingAtAll)
{
    Score score;

    score.read("$: bass.n(\"0\")\n");
    ASSERT_EQ(score.voices().size(), 1U);

    score.read("");

    EXPECT_TRUE(score.voices().empty());
    EXPECT_TRUE(score.problems().empty());
}

// Blanks either side of a line are what an indented document has.
TEST(ScoreTest, TrimsTheBlanksAroundALine)
{
    Score score;

    score.read("  \t$: bell.n(\"0 3\")  \t\n");

    ASSERT_EQ(score.voices().size(), 1U);
    EXPECT_EQ(eventsOn(score, 0), 2U);
    EXPECT_EQ(score.voices()[0].preset, preset("bell"));
    EXPECT_TRUE(score.problems().empty());
}

// The order the lines appear in is the order the voices come back in.
TEST(ScoreTest, HandsBackItsVoicesInLineOrder)
{
    Score score;

    std::string document;

    for (std::size_t at = 0; at < kPresetCount; ++at)
    {
        document += "$: " + std::string(trackName(at)) + ".n(\"0\")\n";
    }

    score.read(document);

    ASSERT_EQ(score.voices().size(), kPresetCount);

    for (std::size_t at = 0; at < kPresetCount; ++at)
    {
        EXPECT_EQ(score.voices()[at].preset, trackPresets()[at]) << at;
    }
}

// A voice being built has to hold something, and silence is it.
TEST(ScoreTest, AVoiceStartsOutSilentRatherThanHoldingNothing)
{
    const Voice voice;

    EXPECT_TRUE(voice.playing.queryAll(kFirstCycle).empty());
    EXPECT_EQ(voice.preset, TrackPreset{});
}

TEST(ScoreTest, AProblemComparesFieldByField)
{
    const Problem problem{.line = 2, .message = "no"};

    EXPECT_EQ(problem, (Problem{.line = 2, .message = "no"}));
    EXPECT_NE(problem, (Problem{.line = 3, .message = "no"}));
    EXPECT_NE(problem, (Problem{.line = 2, .message = "yes"}));
}

// A line never read holds no chain, and neither does a bare mark.
// So the one used to be taken for the other and said nothing.
TEST(ScoreTest, RefusesABareVoiceMarkOnALineNeverReadBefore)
{
    Score score;

    score.read("$:\n");

    ASSERT_EQ(score.problems().size(), 1U);
    EXPECT_EQ(score.problems()[0].line, 1U);
    EXPECT_TRUE(score.voices().empty());
    EXPECT_EQ(score.reparses(), 1U);
}
