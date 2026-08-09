#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <string>

#include <antwika/pattern/Cycle.hpp>
#include <antwika/pattern/Span.hpp>

#include "antwika/music_editor/Score.hpp"
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
}

TEST(ScoreTest, Ctor_StartsSilentAndWithoutComplaint)
{
    const Score score;

    EXPECT_TRUE(score.voices().empty());
    EXPECT_TRUE(score.problems().empty());
    EXPECT_FALSE(score.hasError());
    EXPECT_EQ(score.reparses(), 0U);
}

TEST(ScoreTest, Read_TakesTheOpeningDocumentCleanly)
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

TEST(ScoreTest, OpeningSource_ShowsTheShapeOfTheLanguage)
{
    const auto source = openingSource();

    EXPECT_NE(source.find("$: "), std::string::npos);
    EXPECT_NE(source.find(".n(\""), std::string::npos);
    EXPECT_NE(source.find("<0 10 8 5>"), std::string::npos);
    EXPECT_NE(source.find(".pianoroll()"), std::string::npos);

    const auto first = source.find("$: bass.");
    ASSERT_NE(first, std::string::npos);
    EXPECT_NE(source.find("$: bass.", first + 1), std::string::npos);
}

TEST(ScoreTest, Read_ReadsAVoiceLineIntoAVoice)
{
    Score score;

    score.read("$: bass.n(\"0 3 5\")\n");

    ASSERT_EQ(score.voices().size(), 1U);
    EXPECT_EQ(eventsOn(score, 0), 3U);
    EXPECT_EQ(score.voices()[0].preset, preset("bass"));
    EXPECT_TRUE(score.problems().empty());
}

TEST(ScoreTest, Read_AChainMayRunDownSeveralLines)
{
    Score score;

    score.read(
        "$: bass.n(\"0 3 5\")\n"
        "     .o(-1)\n"
        "     .gain(.2)\n");

    EXPECT_TRUE(score.problems().empty());
    ASSERT_EQ(score.voices().size(), 1U);
    EXPECT_EQ(eventsOn(score, 0), 3U);

    EXPECT_FLOAT_EQ(score.voices()[0].preset.gain, 0.2F);
    EXPECT_EQ(
        score.voices()[0].preset.transpose,
        preset("bass").transpose - 12);
}

TEST(ScoreTest, Read_TakesASpreadChainAsOneVoice)
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

TEST(ScoreTest, Read_SpreadingAChainChangesNothingAboutIt)
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

TEST(ScoreTest, Read_RefusesASpreadChainOnItsOpeningLine)
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

TEST(ScoreTest, Read_ATrappingFractionWordIsAProblemRatherThanDeath)
{
    Score score;

    score.read("$: bass.n(\"-2147483648%-1\")\n");

    ASSERT_EQ(score.problems().size(), 1U);
    EXPECT_EQ(score.problems()[0].line, 1U);
    EXPECT_TRUE(score.voices().empty());
}

TEST(ScoreTest, Read_RefusesACallWithNoVoiceLineAboveIt)
{
    Score score;

    score.read("  .gain(.2)\n$: bass.n(\"0\")\n");

    ASSERT_EQ(score.problems().size(), 1U);
    EXPECT_EQ(score.problems()[0].line, 1U);
    EXPECT_FALSE(score.problems()[0].message.empty());
    ASSERT_EQ(score.voices().size(), 1U);
}

TEST(ScoreTest, Read_KeepsProblemsInLineOrder)
{
    Score score;

    score.read("$: bass.n(\"0\").wobble(3)\nnot a voice line\n");

    ASSERT_EQ(score.problems().size(), 2U);
    EXPECT_EQ(score.problems()[0].line, 1U);
    EXPECT_EQ(score.problems()[1].line, 2U);
}

TEST(ScoreTest, Read_PassesOverACommentAndABlankLine)
{
    Score score;

    score.read("// a comment\n\n   \n\t\n$: lead.n(\"0 3\")\n");

    EXPECT_TRUE(score.problems().empty());
    ASSERT_EQ(score.voices().size(), 1U);
    EXPECT_EQ(eventsOn(score, 0), 2U);
}

TEST(ScoreTest, Read_ReadsAVoiceLineWithACommentAfterIt)
{
    Score score;

    score.read("$: drum.n(\"0(3,8)\")   // the kick\n");

    EXPECT_TRUE(score.problems().empty());
    ASSERT_EQ(score.voices().size(), 1U);
    EXPECT_EQ(eventsOn(score, 0), 3U);
    EXPECT_EQ(score.voices()[0].preset, preset("drum"));
}

TEST(ScoreTest, Read_AWholeLineCommentIsCutAwayLikeAnyOther)
{
    Score score;

    score.read("   // nothing to see\n$: bass.n(\"0\") // nor here\n");

    EXPECT_TRUE(score.problems().empty());
    EXPECT_EQ(score.voices().size(), 1U);
}

TEST(ScoreTest, Read_ACommentMarkInsideQuotesIsNotAComment)
{
    Score score;

    score.read("$: bass.n(\"0 // 3\")\n");

    ASSERT_EQ(score.problems().size(), 1U);
    EXPECT_EQ(score.problems()[0].line, 1U);
    EXPECT_FALSE(score.problems()[0].message.empty());
    EXPECT_TRUE(score.voices().empty());
}

TEST(ScoreTest, Read_ALoneSlashIsNotACommentMark)
{
    Score score;

    score.read("/\n");

    ASSERT_EQ(score.problems().size(), 1U);
    EXPECT_EQ(score.problems()[0].line, 1U);
}

TEST(ScoreTest, Read_TwoLinesOpeningWithOnePresetAreTwoVoices)
{
    Score score;

    score.read(
        "$: drum.n(\"0(3,8)\")\n"
        "$: drum.n(\"0 0\").gain(.12).pan(.5)\n");

    ASSERT_EQ(score.voices().size(), 2U);
    EXPECT_TRUE(score.problems().empty());

    EXPECT_EQ(eventsOn(score, 0), 3U);
    EXPECT_EQ(eventsOn(score, 1), 2U);

    EXPECT_NE(score.voices()[0].preset, score.voices()[1].preset);
    EXPECT_EQ(score.voices()[0].preset, preset("drum"));
    EXPECT_FLOAT_EQ(score.voices()[1].preset.gain, 0.12F);
    EXPECT_FLOAT_EQ(score.voices()[1].preset.pan, 0.5F);
}

TEST(ScoreTest, Read_WhatOneLineChangesDoesNotReachTheNext)
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

TEST(ScoreTest, Read_ALineDeletedTakesItsVoiceWithIt)
{
    Score score;

    score.read("$: bass.n(\"0\")\n$: drum.n(\"0 0\")\n");
    ASSERT_EQ(score.voices().size(), 2U);

    score.read("$: bass.n(\"0\")\n");

    ASSERT_EQ(score.voices().size(), 1U);
    EXPECT_EQ(eventsOn(score, 0), 1U);
    EXPECT_TRUE(score.problems().empty());
}

TEST(ScoreTest, Read_HearsARewrittenLineAgain)
{
    Score score;

    score.read("$: drum.n(\"0 0\")\n");
    score.read("\n");
    ASSERT_TRUE(score.voices().empty());

    score.read("$: drum.n(\"0 0\")\n");

    ASSERT_EQ(score.voices().size(), 1U);
    EXPECT_EQ(eventsOn(score, 0), 2U);
}

TEST(ScoreTest, Read_ARefusedLineKeepsPlayingWhatItLastDid)
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

TEST(ScoreTest, Read_ContributesNoVoiceIfNeverRead)
{
    Score score;

    score.read("$: horn.n(\"0\")\n$: bass.n(\"0 3\")\n");

    ASSERT_EQ(score.problems().size(), 1U);
    EXPECT_EQ(score.problems()[0].line, 1U);

    ASSERT_EQ(score.voices().size(), 1U);
    EXPECT_EQ(eventsOn(score, 0), 2U);
    EXPECT_EQ(score.voices()[0].preset, preset("bass"));
}

TEST(ScoreTest, Read_ReportsWhicheverOfTheThreeRefusalsArrives)
{
    Score chain;
    chain.read("$: n(\"0\").wobble(1)\n");
    EXPECT_EQ(chain.problems().size(), 1U);

    Score grammar;
    grammar.read("$: n(\"0 [\")\n");
    EXPECT_EQ(grammar.problems().size(), 1U);

    Score algebra;
    algebra.read("$: n(\"0(9,8)\")\n");
    EXPECT_EQ(algebra.problems().size(), 1U);
}

TEST(ScoreTest, Read_RefusesALineThatDoesNotOpenWithTheVoiceMark)
{
    Score score;

    score.read("bass.n(\"0 3\")\n");

    ASSERT_EQ(score.problems().size(), 1U);
    EXPECT_EQ(score.problems()[0].line, 1U);
    EXPECT_FALSE(score.problems()[0].message.empty());
    EXPECT_TRUE(score.hasError());
    EXPECT_TRUE(score.voices().empty());
}

TEST(ScoreTest, Read_RefusesALineEmptiedDownToItsVoiceMark)
{
    Score score;

    score.read("$: bass.n(\"0\")\n");
    ASSERT_TRUE(score.problems().empty());

    score.read("$:\n");

    ASSERT_EQ(score.problems().size(), 1U);
    EXPECT_EQ(score.problems()[0].line, 1U);

    ASSERT_EQ(score.voices().size(), 1U);
    EXPECT_EQ(eventsOn(score, 0), 1U);
}

TEST(ScoreTest, Read_TheNewChainTakesOverTheMomentItReads)
{
    Score score;

    score.read("$: bass.n(\"0 3 [\")\n");
    ASSERT_TRUE(score.hasError());

    score.read("$: bass.n(\"0 3 [5 7]\")\n");

    EXPECT_FALSE(score.hasError());
    ASSERT_EQ(score.voices().size(), 1U);
    EXPECT_EQ(eventsOn(score, 0), 4U);
}

TEST(ScoreTest, Read_DropsAComplaintWhenTheLineGoes)
{
    Score score;

    score.read("$: bass.n(\"0 [\")\n");
    ASSERT_TRUE(score.hasError());

    score.read("\n");
    EXPECT_FALSE(score.hasError());

    score.read("$: bass.n(\"0 [\")\n");
    EXPECT_TRUE(score.hasError());
}

TEST(ScoreTest, Read_ARefusalStandsForAsLongAsTheLineDoes)
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

TEST(ScoreTest, Read_ALineWhoseTextDidNotChangeIsNotReadAgain)
{
    Score score;

    score.read("$: bass.n(\"0 3\")\n");
    const auto first = score.reparses();
    ASSERT_EQ(first, 1U);

    score.read("$: bass.n(\"0 3\")\n// and a comment\n");
    EXPECT_EQ(score.reparses(), first);

    score.read("$: bass.n(\"0 3 5\")\n// and a comment\n");
    EXPECT_EQ(score.reparses(), first + 1);
}

TEST(ScoreTest, Read_ADocumentThatDidNotChangeCostsNothing)
{
    Score score;

    score.read("$: bass.n(\"0 3\")\n");
    const auto first = score.reparses();

    score.read("$: bass.n(\"0 3\")\n");

    EXPECT_EQ(score.reparses(), first);
}

TEST(ScoreTest, Read_ReadsALineAgainWhateverWayItChanged)
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

TEST(ScoreTest, Read_CountsLinesFromOne)
{
    Score score;

    score.read("// a comment\nnot a voice line\n$: horn.n(\"0\")\n");

    ASSERT_EQ(score.problems().size(), 2U);
    EXPECT_EQ(score.problems()[0].line, 2U);
    EXPECT_EQ(score.problems()[1].line, 3U);
}

TEST(ScoreTest, Read_ReadsALastLineWithNoBreakAfterIt)
{
    Score score;

    score.read("$: bass.n(\"0 3 5\")");

    ASSERT_EQ(score.voices().size(), 1U);
    EXPECT_EQ(eventsOn(score, 0), 3U);
    EXPECT_TRUE(score.problems().empty());
}

TEST(ScoreTest, Read_ReadsAnEmptyDocumentAsNothingAtAll)
{
    Score score;

    score.read("$: bass.n(\"0\")\n");
    ASSERT_EQ(score.voices().size(), 1U);

    score.read("");

    EXPECT_TRUE(score.voices().empty());
    EXPECT_TRUE(score.problems().empty());
}

TEST(ScoreTest, Read_TrimsTheBlanksAroundALine)
{
    Score score;

    score.read("  \t$: bell.n(\"0 3\")  \t\n");

    ASSERT_EQ(score.voices().size(), 1U);
    EXPECT_EQ(eventsOn(score, 0), 2U);
    EXPECT_EQ(score.voices()[0].preset, preset("bell"));
    EXPECT_TRUE(score.problems().empty());
}

TEST(ScoreTest, Read_HandsBackItsVoicesInLineOrder)
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

TEST(ScoreTest, Voices_StartOutSilentRatherThanEmpty)
{
    const Voice voice;

    EXPECT_TRUE(voice.playing.queryAll(kFirstCycle).empty());
    EXPECT_EQ(voice.preset, TrackPreset{});
}

TEST(ScoreTest, OperatorEquals_ComparesProblemsFieldByField)
{
    const Problem problem{.line = 2, .message = "no"};

    EXPECT_EQ(problem, (Problem{.line = 2, .message = "no"}));
    EXPECT_NE(problem, (Problem{.line = 3, .message = "no"}));
    EXPECT_NE(problem, (Problem{.line = 2, .message = "yes"}));
}

TEST(ScoreTest, Read_RefusesABareVoiceMarkOnALineNeverReadBefore)
{
    Score score;

    score.read("$:\n");

    ASSERT_EQ(score.problems().size(), 1U);
    EXPECT_EQ(score.problems()[0].line, 1U);
    EXPECT_TRUE(score.voices().empty());
    EXPECT_EQ(score.reparses(), 1U);
}

TEST(ScoreTest, SpanIn_MapsANoteSpanOntoTheDocument)
{
    Score score;

    const std::string source = "// intro\n$: drum.n(\"0 ~ 3\")\n";

    score.read(source);

    const auto span = score.spanIn(0, 4, 1);

    ASSERT_TRUE(span.has_value());
    EXPECT_EQ(source.substr(span->begin, span->end - span->begin), "3");
}

TEST(ScoreTest, SpanIn_MapsASpanThroughAContinuationLine)
{
    Score score;

    const std::string source = "$: drum\n    .n(\"0 5\")\n";

    score.read(source);

    const auto span = score.spanIn(0, 2, 1);

    ASSERT_TRUE(span.has_value());
    EXPECT_EQ(source.substr(span->begin, span->end - span->begin), "5");
}

TEST(ScoreTest, SpanIn_SpansFollowTheDocumentAsLinesMoveIt)
{
    Score score;

    score.read("$: drum.n(\"7\")\n");

    const std::string moved = "// a comment above\n$: drum.n(\"7\")\n";

    score.read(moved);

    const auto span = score.spanIn(0, 0, 1);

    ASSERT_TRUE(span.has_value());
    EXPECT_EQ(moved.substr(span->begin, span->end - span->begin), "7");
}

TEST(ScoreTest, SpanIn_AVoiceThatIsGoneHasNoSpanToGive)
{
    Score score;

    score.read("$: drum.n(\"0\")\n");

    EXPECT_FALSE(score.spanIn(3, 0, 1).has_value());
}

TEST(ScoreTest, SpanIn_ASpanPastTheChainIsDropped)
{
    Score score;

    score.read("$: drum.n(\"0\")\n");

    EXPECT_FALSE(score.spanIn(0, 9999, 1).has_value());
}

TEST(ScoreTest, SpanIn_ASpanIsClampedToItsOwnStretch)
{
    Score score;

    const std::string source = "$: drum.n(\"0 5\")\n";

    score.read(source);

    const auto span = score.spanIn(0, 2, 999);

    ASSERT_TRUE(span.has_value());

    EXPECT_EQ(
        source.substr(span->begin, span->end - span->begin), "5\")");
}

TEST(ScoreTest, SpanIn_ASpanOfNothingIsDropped)
{
    Score score;

    score.read("$: drum.n(\"0\")\n");

    EXPECT_FALSE(score.spanIn(0, 0, 0).has_value());
}

TEST(ScoreTest, OperatorEquals_ComparesSpansEndByEnd)
{
    using antwika::music_editor::DocumentSpan;

    constexpr DocumentSpan span{.begin = 2, .end = 5};

    EXPECT_EQ(span, (DocumentSpan{.begin = 2, .end = 5}));
    EXPECT_NE(span, (DocumentSpan{.begin = 3, .end = 5}));
    EXPECT_NE(span, (DocumentSpan{.begin = 2, .end = 6}));
}

namespace
{
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
}

TEST(ScoreTest, Read_AFormSchedulesAPartBlocksVoices)
{
    Score score;

    score.read(arrangedSource());

    EXPECT_TRUE(score.problems().empty());
    ASSERT_EQ(score.voices().size(), 3U);

    for (std::int64_t cycle = 0; cycle < 7; ++cycle)
    {
        EXPECT_EQ(onsetsAt(score, 0, cycle), 1U);

        const bool verse = cycle >= 2 && cycle < 5;

        EXPECT_EQ(onsetsAt(score, 1, cycle), verse ? 1U : 0U);
        EXPECT_EQ(onsetsAt(score, 2, cycle), verse ? 0U : 1U);
    }

    EXPECT_EQ(onsetsAt(score, 2, 7), 1U);
    EXPECT_EQ(onsetsAt(score, 1, 9), 1U);
}

TEST(ScoreTest, Read_SilencesPartVoicesUntilScheduled)
{
    Score score;

    score.read(
        "$: drum.n(\"0\")\n"
        "part: verse\n"
        "$: bass.n(\"3\")\n");

    ASSERT_EQ(score.voices().size(), 1U);
    ASSERT_EQ(score.problems().size(), 1U);
    EXPECT_EQ(score.problems()[0].line, 2U);
    EXPECT_EQ(
        score.problems()[0].message,
        "no form: says when these parts play");
}

TEST(ScoreTest, Read_SilencesAnUnplayedPartQuietly)
{
    Score score;

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

TEST(ScoreTest, Read_AFormNameWithNoPartBlockIsAProblemOnce)
{
    Score score;

    score.read(
        "form: verse verse\n"
        "bars: 2\n");

    ASSERT_EQ(score.problems().size(), 1U);
    EXPECT_EQ(score.problems()[0].line, 1U);
    EXPECT_EQ(score.problems()[0].message, "no part: holds verse");
}

TEST(ScoreTest, Read_AnEmptyPartBlockIsLegalSilence)
{
    Score score;

    score.read(
        "form: verse breakdown\n"
        "bars: 2\n"
        "part: verse\n"
        "$: bass.n(\"3\")\n"
        "part: breakdown\n");

    EXPECT_TRUE(score.problems().empty());
    EXPECT_EQ(score.voices().size(), 1U);
}

TEST(ScoreTest, Read_ASecondFormOrBarsLineIsRefused)
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

    EXPECT_EQ(score.voices().size(), 1U);
    EXPECT_EQ(onsetsAt(score, 0, 0), 1U);
    EXPECT_EQ(onsetsAt(score, 0, 2), 1U);
}

TEST(ScoreTest, Read_KeepsTheLastFormWhenARewriteFails)
{
    Score score;

    score.read(arrangedSource());
    ASSERT_EQ(score.voices().size(), 3U);

    auto broken = arrangedSource();
    broken.replace(0, 5, "form: !!");

    score.read(broken);

    EXPECT_EQ(score.problems().size(), 1U);
    ASSERT_EQ(score.voices().size(), 3U);
    EXPECT_EQ(onsetsAt(score, 1, 2), 1U);
    EXPECT_EQ(onsetsAt(score, 1, 0), 0U);
}

TEST(ScoreTest, Read_DeletingTheFormSilencesThePartBlocks)
{
    Score score;

    score.read(arrangedSource());
    ASSERT_EQ(score.voices().size(), 3U);

    const auto source = arrangedSource();
    score.read(source.substr(source.find('\n') + 1));

    ASSERT_EQ(score.voices().size(), 1U);
    ASSERT_EQ(score.problems().size(), 1U);
    EXPECT_EQ(
        score.problems()[0].message,
        "no form: says when these parts play");
}

TEST(ScoreTest, Read_AMalformedPartHeaderSilencesItsBlock)
{
    Score score;

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

TEST(ScoreTest, Read_AnUnmarkedNameWithNoBarsLineIsAProblem)
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

TEST(ScoreTest, Read_AFormOfMarkedNamesNeedsNoBarsLine)
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

TEST(ScoreTest, Read_AHeaderTakesACommentLikeAnyOtherLine)
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

TEST(ScoreTest, Read_AHeaderEndsTheChainAboveIt)
{
    Score score;

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

TEST(ScoreTest, Read_ChangingTheFormAloneReparsesNoChain)
{
    Score score;

    score.read(arrangedSource());

    const auto parsed = score.reparses();

    auto retimed = arrangedSource();
    retimed.replace(retimed.find("verse/1"), 7, "verse/2");

    score.read(retimed);

    EXPECT_EQ(score.reparses(), parsed);
    EXPECT_TRUE(score.problems().empty());
    EXPECT_EQ(onsetsAt(score, 1, 5), 1U);
}

TEST(ScoreTest, Read_TwoBlocksMayHoldTheSameSection)
{
    Score score;

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

TEST(ScoreTest, Read_KeepsTheLastBarsLineOnAFailure)
{
    Score score;

    score.read(arrangedSource());
    ASSERT_EQ(score.voices().size(), 3U);

    auto broken = arrangedSource();
    broken.replace(broken.find("bars: 2"), 7, "bars: two");

    score.read(broken);

    ASSERT_EQ(score.problems().size(), 1U);
    EXPECT_EQ(score.problems()[0].line, 2U);

    ASSERT_EQ(score.voices().size(), 3U);
    EXPECT_EQ(onsetsAt(score, 1, 2), 1U);
    EXPECT_EQ(onsetsAt(score, 1, 0), 0U);
}

TEST(ScoreTest, Read_ReadsEveryDocumentedModulationExample)
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

TEST(ScoreTest, Pianorolls_APianorollHangsUnderTheLineThatAsksForIt)
{
    Score score;

    score.read(
        "$: drum.n(\"0 3\").pianoroll()\n"
        "$: bass.n(\"0\")\n");

    ASSERT_EQ(score.pianorolls().size(), 1U);
    EXPECT_EQ(score.pianorolls()[0].line, 0U);
    EXPECT_EQ(score.pianorolls()[0].preset, preset("drum"));
    EXPECT_EQ(
        score.pianorolls()[0].playing.queryAll(kFirstCycle).size(), 2U);
}

TEST(ScoreTest, Pianorolls_ALineWithoutTheCallHangsNoRoll)
{
    Score score;

    score.read("$: drum.n(\"0 3\")\n");

    EXPECT_TRUE(score.pianorolls().empty());
}

TEST(ScoreTest, Pianorolls_ASpreadChainHangsItsRollUnderItsLastLine)
{
    Score score;

    score.read(
        "// a comment above\n"
        "$: drum.n(\"0\")\n"
        "    .pianoroll()\n");

    ASSERT_EQ(score.pianorolls().size(), 1U);
    EXPECT_EQ(score.pianorolls()[0].line, 2U);
}

TEST(ScoreTest, Pianorolls_APartsVoiceStillHangsItsOwnRoll)
{
    Score score;

    score.read(
        "form: a\n"
        "bars: 1\n"
        "part: a\n"
        "part: b\n"
        "$: drum.n(\"0\").pianoroll()\n");

    ASSERT_EQ(score.pianorolls().size(), 1U);
    EXPECT_EQ(score.pianorolls()[0].line, 4U);

    EXPECT_EQ(
        score.pianorolls()[0].playing.queryAll(kFirstCycle).size(), 1U);
}

TEST(ScoreTest, Pianorolls_ARollOutlivesAnEditThatWillNotRead)
{
    Score score;

    score.read("$: drum.n(\"0\").pianoroll()\n");

    score.read(
        "// pushed down a line\n"
        "$: drum.n(\"0\").pianoroll().wobble(\n");

    ASSERT_EQ(score.pianorolls().size(), 1U);
    EXPECT_EQ(score.pianorolls()[0].line, 1U);
    EXPECT_TRUE(score.hasError());
}

TEST(ScoreTest, Pianorolls_RemovingTheCallTakesTheRollWithIt)
{
    Score score;

    score.read("$: drum.n(\"0\").pianoroll()\n");
    ASSERT_EQ(score.pianorolls().size(), 1U);

    score.read("$: drum.n(\"0\")\n");
    EXPECT_TRUE(score.pianorolls().empty());
}

TEST(ScoreTest, Pianorolls_AnEmptiedChainDropsItsRoll)
{
    Score score;

    score.read("$: drum.n(\"0\").pianoroll()\n");
    ASSERT_EQ(score.pianorolls().size(), 1U);

    score.read("$:\n");
    EXPECT_TRUE(score.pianorolls().empty());
}

TEST(ScoreTest, Pianorolls_AWaveformHangsUnderTheLineThatAsksForIt)
{
    Score score;

    score.read(
        "$: bass.n(\"0\")\n"
        "$: lead.n(\"0 3\").waveform()\n");

    ASSERT_EQ(score.waveforms().size(), 1U);
    EXPECT_EQ(score.waveforms()[0].line, 1U);
    EXPECT_EQ(score.waveforms()[0].preset, preset("lead"));
    EXPECT_EQ(
        score.waveforms()[0].playing.queryAll(kFirstCycle).size(), 2U);
    EXPECT_TRUE(score.pianorolls().empty());
}

TEST(ScoreTest, Pianorolls_ALineMayAskForBothPicturesAtOnce)
{
    Score score;

    score.read("$: drum.n(\"0\").pianoroll().waveform()\n");

    ASSERT_EQ(score.pianorolls().size(), 1U);
    ASSERT_EQ(score.waveforms().size(), 1U);
    EXPECT_EQ(score.pianorolls()[0].line, 0U);
    EXPECT_EQ(score.waveforms()[0].line, 0U);
}

TEST(ScoreTest, Waveforms_AnEmptiedChainDropsItsWaveToo)
{
    Score score;

    score.read("$: drum.n(\"0\").waveform()\n");
    ASSERT_EQ(score.waveforms().size(), 1U);

    score.read("$:\n");
    EXPECT_TRUE(score.waveforms().empty());
}
