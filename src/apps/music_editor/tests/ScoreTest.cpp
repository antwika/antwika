#include "antwika/music_editor/Score.hpp"

#include <cstddef>
#include <cstdint>
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

    // And a chain carried onto a second line.
    // A dot reads as a continuation once somebody has seen one.
    EXPECT_NE(source.find("\n    ."), std::string::npos);
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

// A chain may run down as many lines as it likes.
// A line opening with a dot is the one above it carrying on.
TEST(ScoreTest, AChainMayRunDownSeveralLines)
{
    Score score;

    score.read(
        "$: bass.n(\"0 3 5\")\n"
        "     .o(-1)\n"
        "     .gain(.2)\n");

    EXPECT_TRUE(score.problems().empty());
    ASSERT_EQ(score.voices().size(), 1U);
    EXPECT_EQ(eventsOn(score, 0), 3U);

    // Every call took, however far down the chain it was written.
    EXPECT_FLOAT_EQ(score.voices()[0].preset.gain, 0.2F);
    EXPECT_EQ(
        score.voices()[0].preset.transpose,
        preset("bass").transpose - 12);
}

// One voice however many lines it took.
// And the lines after it are still their own voices.
TEST(ScoreTest, ASpreadChainIsOneVoiceAndTheNextLineIsAnother)
{
    Score score;

    score.read(
        "$: bass.n(\"0\")\n"
        "  .o(-1)\n"
        "$: lead.n(\"3 5\")\n");

    EXPECT_TRUE(score.problems().empty());
    ASSERT_EQ(score.voices().size(), 2U);
    EXPECT_EQ(eventsOn(score, 0), 1U);
    EXPECT_EQ(eventsOn(score, 1), 2U);
}

// The same chain, written both ways, is the same voice.
TEST(ScoreTest, SpreadingAChainChangesNothingAboutIt)
{
    Score spread;
    Score together;

    spread.read(
        "$: lead.n(\"0 3\")\n"
        "  .o(1)\n"
        "  .gain(.5)\n");

    together.read("$: lead.n(\"0 3\").o(1).gain(.5)\n");

    ASSERT_EQ(spread.voices().size(), 1U);
    ASSERT_EQ(together.voices().size(), 1U);
    EXPECT_EQ(spread.voices()[0].preset, together.voices()[0].preset);
}

// A refusal names the line the voice opened on.
// That is the one a reader has to go and look at.
TEST(ScoreTest, ASpreadChainIsRefusedAgainstTheLineItOpenedOn)
{
    Score score;

    score.read(
        "// a comment\n"
        "$: bass.n(\"0\")\n"
        "  .wobble(3)\n");

    ASSERT_EQ(score.problems().size(), 1U);
    EXPECT_EQ(score.problems()[0].line, 2U);
    EXPECT_TRUE(score.voices().empty());
}

// The scaled minimum over minus one used to be a hardware trap.
// No typed line may ever end the editor; this one is refused instead.
TEST(ScoreTest, ATrappingFractionWordIsAProblemRatherThanDeath)
{
    Score score;

    score.read("$: bass.n(\"-2147483648%-1\")\n");

    ASSERT_EQ(score.problems().size(), 1U);
    EXPECT_EQ(score.problems()[0].line, 1U);
    EXPECT_TRUE(score.voices().empty());
}

TEST(ScoreTest, RefusesACallWithNoVoiceLineAboveIt)
{
    Score score;

    score.read("  .gain(.2)\n$: bass.n(\"0\")\n");

    ASSERT_EQ(score.problems().size(), 1U);
    EXPECT_EQ(score.problems()[0].line, 1U);
    EXPECT_FALSE(score.problems()[0].message.empty());
    ASSERT_EQ(score.voices().size(), 1U);
}

// Whatever ends a voice comes after it.
// So the problems stay in the order their lines are in.
TEST(ScoreTest, KeepsTheProblemsInLineOrderAcrossAVoiceThatEnds)
{
    Score score;

    score.read("$: bass.n(\"0\").wobble(3)\nnot a voice line\n");

    ASSERT_EQ(score.problems().size(), 2U);
    EXPECT_EQ(score.problems()[0].line, 1U);
    EXPECT_EQ(score.problems()[1].line, 2U);
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

// A note's span maps back onto the very characters in the document.
TEST(ScoreTest, MapsANoteSpanOntoTheDocument)
{
    Score score;

    const std::string source = "// intro\n$: drum.n(\"0 ~ 3\")\n";

    score.read(source);

    // The 3 sits four characters into the notation.
    const auto span = score.spanIn(0, 4, 1);

    ASSERT_TRUE(span.has_value());
    EXPECT_EQ(source.substr(span->begin, span->end - span->begin), "3");
}

// A chain spread over lines maps each stretch to its own line.
TEST(ScoreTest, MapsASpanThroughAContinuationLine)
{
    Score score;

    const std::string source = "$: drum\n    .n(\"0 5\")\n";

    score.read(source);

    const auto span = score.spanIn(0, 2, 1);

    ASSERT_TRUE(span.has_value());
    EXPECT_EQ(source.substr(span->begin, span->end - span->begin), "5");
}

// Writing a line above moves every span with the text it lights.
TEST(ScoreTest, SpansFollowTheDocumentAsLinesMoveIt)
{
    Score score;

    score.read("$: drum.n(\"7\")\n");

    const std::string moved = "// a comment above\n$: drum.n(\"7\")\n";

    score.read(moved);

    const auto span = score.spanIn(0, 0, 1);

    ASSERT_TRUE(span.has_value());
    EXPECT_EQ(moved.substr(span->begin, span->end - span->begin), "7");
}

TEST(ScoreTest, AVoiceThatIsGoneHasNoSpanToGive)
{
    Score score;

    score.read("$: drum.n(\"0\")\n");

    EXPECT_FALSE(score.spanIn(3, 0, 1).has_value());
}

// An offset past what the line reads points at nothing honest.
TEST(ScoreTest, ASpanPastTheChainIsDropped)
{
    Score score;

    score.read("$: drum.n(\"0\")\n");

    EXPECT_FALSE(score.spanIn(0, 9999, 1).has_value());
}

// A length past the stretch's edge lights what it can.
TEST(ScoreTest, ASpanIsClampedToItsOwnStretch)
{
    Score score;

    const std::string source = "$: drum.n(\"0 5\")\n";

    score.read(source);

    const auto span = score.spanIn(0, 2, 999);

    ASSERT_TRUE(span.has_value());

    // From the 5 to the stretch's end, and no further.
    EXPECT_EQ(
        source.substr(span->begin, span->end - span->begin), "5\")");
}

TEST(ScoreTest, ASpanOfNothingIsDropped)
{
    Score score;

    score.read("$: drum.n(\"0\")\n");

    EXPECT_FALSE(score.spanIn(0, 0, 0).has_value());
}

TEST(ScoreTest, ComparesSpansEndByEnd)
{
    using antwika::music_editor::DocumentSpan;

    constexpr DocumentSpan span{.begin = 2, .end = 5};

    EXPECT_EQ(span, (DocumentSpan{.begin = 2, .end = 5}));
    EXPECT_NE(span, (DocumentSpan{.begin = 3, .end = 5}));
    EXPECT_NE(span, (DocumentSpan{.begin = 2, .end = 6}));
}

namespace
{
    // A document arranged into sections, small enough to read whole.
    // Two bars of intro, two of verse, one more verse bar, two out.
    [[nodiscard]] std::string arrangedSource()
    {
        return "form: intro verse verse/1 outro\n"
               "bars: 2\n"
               "$: drum.n(\"0\")\n"
               "part: verse\n"
               "$: bass.n(\"3\")\n"
               "part: intro outro\n"
               "$: bell.n(\"12\")\n";
    }

    // How many onsets a voice puts in one whole cycle.
    [[nodiscard]] std::size_t onsetsAt(
        const Score &score,
        const std::size_t voice,
        const std::int64_t cycle)
    {
        std::size_t onsets = 0;

        for (const auto &hap : score.voices()[voice].playing.queryAll(
                 Span(Cycle(cycle), Cycle(cycle + 1))))
        {
            if (hap.hasOnset())
            {
                ++onsets;
            }
        }

        return onsets;
    }
} // namespace

TEST(ScoreTest, AFormSchedulesAPartBlocksVoices)
{
    Score score;

    score.read(arrangedSource());

    EXPECT_TRUE(score.problems().empty());
    ASSERT_EQ(score.voices().size(), 3U);

    // The always-on drum, every cycle of the seven-cycle form.
    // The bass through the verses, the bell through the bookends.
    for (std::int64_t cycle = 0; cycle < 7; ++cycle)
    {
        EXPECT_EQ(onsetsAt(score, 0, cycle), 1U);

        const bool verse = cycle >= 2 && cycle < 5;

        EXPECT_EQ(onsetsAt(score, 1, cycle), verse ? 1U : 0U);
        EXPECT_EQ(onsetsAt(score, 2, cycle), verse ? 0U : 1U);
    }

    // And the whole form comes round again.
    EXPECT_EQ(onsetsAt(score, 2, 7), 1U);
    EXPECT_EQ(onsetsAt(score, 1, 9), 1U);
}

TEST(ScoreTest, PartVoicesAreSilentUntilAFormSchedulesThem)
{
    Score score;

    score.read(
        "$: drum.n(\"0\")\n"
        "part: verse\n"
        "$: bass.n(\"3\")\n");

    // The block's voice is out, and the header says what is missing.
    ASSERT_EQ(score.voices().size(), 1U);
    ASSERT_EQ(score.problems().size(), 1U);
    EXPECT_EQ(score.problems()[0].line, 2U);
    EXPECT_EQ(
        score.problems()[0].message,
        "no form: says when these parts play");
}

TEST(ScoreTest, APartTheFormNeverPlaysIsSilentWithoutComplaint)
{
    Score score;

    // The solo workflow: the form names one section to write in.
    score.read(
        "form: verse\n"
        "bars: 2\n"
        "part: verse\n"
        "$: bass.n(\"3\")\n"
        "part: chorus\n"
        "$: lead.n(\"7\")\n");

    EXPECT_TRUE(score.problems().empty());
    ASSERT_EQ(score.voices().size(), 1U);
    EXPECT_EQ(onsetsAt(score, 0, 0), 1U);
}

TEST(ScoreTest, AFormNameWithNoPartBlockIsAProblemOnce)
{
    Score score;

    score.read(
        "form: verse verse\n"
        "bars: 2\n");

    ASSERT_EQ(score.problems().size(), 1U);
    EXPECT_EQ(score.problems()[0].line, 1U);
    EXPECT_EQ(score.problems()[0].message, "no part: holds verse");
}

TEST(ScoreTest, AnEmptyPartBlockIsLegalSilence)
{
    Score score;

    // A breakdown is written as exactly that: a block with nothing.
    score.read(
        "form: verse breakdown\n"
        "bars: 2\n"
        "part: verse\n"
        "$: bass.n(\"3\")\n"
        "part: breakdown\n");

    EXPECT_TRUE(score.problems().empty());
    EXPECT_EQ(score.voices().size(), 1U);
}

TEST(ScoreTest, ASecondFormOrBarsLineIsRefused)
{
    Score score;

    score.read(
        "form: a\n"
        "bars: 2\n"
        "form: b\n"
        "bars: 4\n"
        "part: a\n"
        "$: bass.n(\"3\")\n");

    ASSERT_EQ(score.problems().size(), 2U);
    EXPECT_EQ(score.problems()[0].message, "one form: per score");
    EXPECT_EQ(score.problems()[1].message, "one bars: per score");

    // The first of each stands, so the voice still plays.
    EXPECT_EQ(score.voices().size(), 1U);
    EXPECT_EQ(onsetsAt(score, 0, 0), 1U);
    EXPECT_EQ(onsetsAt(score, 0, 2), 1U);
}

TEST(ScoreTest, ARewrittenFormThatRefusesKeepsTheLastOneArranging)
{
    Score score;

    score.read(arrangedSource());
    ASSERT_EQ(score.voices().size(), 3U);

    // The form line half retyped: a refusal, and nothing moves.
    auto broken = arrangedSource();
    broken.replace(0, 5, "form: !!");

    score.read(broken);

    EXPECT_EQ(score.problems().size(), 1U);
    ASSERT_EQ(score.voices().size(), 3U);
    EXPECT_EQ(onsetsAt(score, 1, 2), 1U);
    EXPECT_EQ(onsetsAt(score, 1, 0), 0U);
}

TEST(ScoreTest, DeletingTheFormSilencesThePartBlocks)
{
    Score score;

    score.read(arrangedSource());
    ASSERT_EQ(score.voices().size(), 3U);

    // The form line cut whole: an intended cut, not a half-typed one.
    const auto source = arrangedSource();
    score.read(source.substr(source.find('\n') + 1));

    ASSERT_EQ(score.voices().size(), 1U);
    ASSERT_EQ(score.problems().size(), 1U);
    EXPECT_EQ(
        score.problems()[0].message,
        "no form: says when these parts play");
}

TEST(ScoreTest, AMalformedPartHeaderSilencesItsBlock)
{
    Score score;

    // The header will not read, so its voices are silent.
    // Merged into the block above they would sound in its sections.
    score.read(
        "form: verse\n"
        "bars: 2\n"
        "part: verse\n"
        "$: bass.n(\"3\")\n"
        "part: !\n"
        "$: lead.n(\"7\")\n");

    ASSERT_EQ(score.problems().size(), 1U);
    EXPECT_EQ(score.problems()[0].line, 5U);
    EXPECT_EQ(score.voices().size(), 1U);
}

TEST(ScoreTest, AnUnmarkedNameWithNoBarsLineIsAProblem)
{
    Score score;

    score.read(
        "form: verse\n"
        "part: verse\n"
        "$: bass.n(\"3\")\n");

    ASSERT_EQ(score.problems().size(), 1U);
    EXPECT_EQ(score.problems()[0].line, 1U);
    EXPECT_EQ(
        score.problems()[0].message,
        "verse has no length: give a bars: line or verse/<n>");
    EXPECT_TRUE(score.voices().empty());
}

TEST(ScoreTest, AFormOfMarkedNamesNeedsNoBarsLine)
{
    Score score;

    score.read(
        "form: verse/2\n"
        "part: verse\n"
        "$: bass.n(\"3\")\n");

    EXPECT_TRUE(score.problems().empty());
    ASSERT_EQ(score.voices().size(), 1U);
    EXPECT_EQ(onsetsAt(score, 0, 0), 1U);
}

TEST(ScoreTest, AHeaderTakesACommentLikeAnyOtherLine)
{
    Score score;

    score.read(
        "form: verse // the whole song, for now\n"
        "bars: 2 // two cycles each\n"
        "part: verse // the one block\n"
        "$: bass.n(\"3\")\n");

    EXPECT_TRUE(score.problems().empty());
    EXPECT_EQ(score.voices().size(), 1U);
}

TEST(ScoreTest, AHeaderEndsTheChainAboveIt)
{
    Score score;

    // The continuation lands under the header, above no voice line.
    score.read(
        "$: bass.n(\"3\")\n"
        "part: verse\n"
        "    .gain(.2)\n");

    ASSERT_EQ(score.problems().size(), 2U);
    EXPECT_EQ(score.problems()[0].line, 2U);
    EXPECT_EQ(
        score.problems()[0].message,
        "no form: says when these parts play");
    EXPECT_EQ(score.problems()[1].line, 3U);
    EXPECT_EQ(
        score.problems()[1].message, "a call above no voice line");
}

TEST(ScoreTest, ChangingTheFormAloneReparsesNoChain)
{
    Score score;

    score.read(arrangedSource());

    const auto parsed = score.reparses();

    auto retimed = arrangedSource();
    retimed.replace(retimed.find("verse/1"), 7, "verse/2");

    score.read(retimed);

    // The live feel: an arrangement edit re-reads no voice line.
    EXPECT_EQ(score.reparses(), parsed);
    EXPECT_TRUE(score.problems().empty());
    EXPECT_EQ(onsetsAt(score, 1, 5), 1U);
}

TEST(ScoreTest, TwoBlocksMayHoldTheSameSection)
{
    Score score;

    // Two part: blocks naming verse are both verse material.
    score.read(
        "form: verse\n"
        "bars: 2\n"
        "part: verse\n"
        "$: bass.n(\"3\")\n"
        "part: verse\n"
        "$: lead.n(\"7\")\n");

    EXPECT_TRUE(score.problems().empty());
    ASSERT_EQ(score.voices().size(), 2U);
    EXPECT_EQ(onsetsAt(score, 0, 0), 1U);
    EXPECT_EQ(onsetsAt(score, 1, 0), 1U);
}

TEST(ScoreTest, ARewrittenBarsLineThatRefusesKeepsTheLastOne)
{
    Score score;

    score.read(arrangedSource());
    ASSERT_EQ(score.voices().size(), 3U);

    auto broken = arrangedSource();
    broken.replace(broken.find("bars: 2"), 7, "bars: two");

    score.read(broken);

    ASSERT_EQ(score.problems().size(), 1U);
    EXPECT_EQ(score.problems()[0].line, 2U);

    // The last bars that read keeps the sections their length.
    ASSERT_EQ(score.voices().size(), 3U);
    EXPECT_EQ(onsetsAt(score, 1, 2), 1U);
    EXPECT_EQ(onsetsAt(score, 1, 0), 0U);
}

// The wiki's example scores, read by the real parser.
// A documented chain that will not read is a documentation bug.
TEST(ScoreTest, ReadsEveryDocumentedModulationExample)
{
    Score score;

    score.read(
        "$: drum.s(sine).base(150).slide(-600).hold(90).rel(80)"
        ".n(\"0 ~ 0 ~\")\n"
        "$: drum.bpf(2000).dec(60).hold(70).gain(.25).n(\"~ 0 ~ 0\")\n"
        "$: drum.hpf(8000).hold(25).rel(30).gain(.15).n(\"0 0 0 0\")\n"
        "$: drum.hpf(5000).hold(60).rel(900).gain(.2).n(\"0 ~ ~ ~\")\n");
    EXPECT_EQ(score.voices().size(), 4U);

    score.read(
        "$: lead.s(triangle).att(2).dec(250).sus(.3).rel(200)"
        ".n(\"0 4 7 12\")\n"
        "$: bass.o(-1).n(\"0 ~ 0 3\")\n"
        "$: lead.s(saw).lpf(1800).dec(180).sus(.2).rel(120).harm(12)"
        ".n(\"0 7\")\n");
    EXPECT_EQ(score.voices().size(), 3U);

    score.read(
        "$: bell.n(\"[0,3,7] [8,12,15] [3,7,10] [10,14,17]\")"
        ".gain(.25)\n");
    EXPECT_EQ(score.voices().size(), 1U);

    score.read(
        "$: lead.vib(6).delay(250).delaymix(.3).harm(7)"
        ".n(\"0 ~ 3 ~\").gain(.2)\n");
    EXPECT_EQ(score.voices().size(), 1U);
}
