#include "antwika/music_editor/Score.hpp"

#include <array>
#include <string>

#include <gtest/gtest.h>

#include <antwika/pattern/Cycle.hpp>
#include <antwika/pattern/Span.hpp>

#include "antwika/music_editor/TrackPreset.hpp"

using antwika::music_editor::kTrackCount;
using antwika::music_editor::Score;
using antwika::pattern::Cycle;
using antwika::pattern::Span;

namespace
{
    using Lines = std::array<std::string, kTrackCount>;

    const Span kFirstCycle(Cycle(), Cycle(1));

    [[nodiscard]] std::size_t eventsOn(
        const Score &score, std::size_t track)
    {
        return score.playing(track).queryAll(kFirstCycle).size();
    }
} // namespace

TEST(ScoreTest, StartsSilentAndWithoutComplaint)
{
    const Score score;

    for (std::size_t track = 0; track < kTrackCount; ++track)
    {
        EXPECT_EQ(eventsOn(score, track), 0U) << track;
        EXPECT_TRUE(score.error(track).empty()) << track;
    }

    EXPECT_FALSE(score.hasError());
}

TEST(ScoreTest, ReadsALineIntoWhatItPlays)
{
    Score score;
    Lines lines;
    lines[0] = "0 3 5";

    score.update(lines);

    EXPECT_EQ(eventsOn(score, 0), 3U);
    EXPECT_TRUE(score.error(0).empty());
}

TEST(ScoreTest, ABlankLineIsSilentRatherThanRefused)
{
    Score score;
    Lines lines;
    lines[0] = "0";
    score.update(lines);

    lines[0] = "   ";
    score.update(lines);

    EXPECT_EQ(eventsOn(score, 0), 0U);
    EXPECT_TRUE(score.error(0).empty());
}

// The decision the whole feel of the editor rests on.
// Half a bracket is typed on the way to a whole one.
TEST(ScoreTest, ARefusedLineKeepsPlayingWhatItLastDid)
{
    Score score;
    Lines lines;

    lines[0] = "0 3 5";
    score.update(lines);
    ASSERT_EQ(eventsOn(score, 0), 3U);

    lines[0] = "0 3 5 [";
    score.update(lines);

    EXPECT_EQ(eventsOn(score, 0), 3U);
    EXPECT_FALSE(score.error(0).empty());
    EXPECT_TRUE(score.hasError());
}

TEST(ScoreTest, TheNewPatternTakesOverTheMomentItReads)
{
    Score score;
    Lines lines;

    lines[0] = "0 3 5 [";
    score.update(lines);
    ASSERT_FALSE(score.error(0).empty());

    lines[0] = "0 3 5 [7 9]";
    score.update(lines);

    EXPECT_EQ(eventsOn(score, 0), 5U);
    EXPECT_TRUE(score.error(0).empty());
    EXPECT_FALSE(score.hasError());
}

// A line may parse cleanly and ask for something impossible.
// That reads the same here as one that does not parse at all.
TEST(ScoreTest, TheAlgebrasRefusalIsReportedToo)
{
    Score score;
    Lines lines;
    lines[1] = "0(9,8)";

    score.update(lines);

    EXPECT_FALSE(score.error(1).empty());
    EXPECT_EQ(eventsOn(score, 1), 0U);
}

// Cheap on a tick where nothing was typed, which is nearly every tick.
TEST(ScoreTest, ALineThatDidNotChangeIsNotReadAgain)
{
    Score score;
    Lines lines;
    lines[0] = "0 3";

    score.update(lines);
    const auto first = score.reparses();

    score.update(lines);

    EXPECT_EQ(score.reparses(), first);

    lines[0] = "0 3 5";
    score.update(lines);

    EXPECT_EQ(score.reparses(), first + 1);
}

TEST(ScoreTest, EachLineIsReadOnItsOwn)
{
    Score score;
    Lines lines;
    lines[0] = "0";
    lines[2] = "[";

    score.update(lines);

    EXPECT_EQ(eventsOn(score, 0), 1U);
    EXPECT_TRUE(score.error(0).empty());
    EXPECT_FALSE(score.error(2).empty());
}

TEST(ScoreTest, TreatsATabAsBlankToo)
{
    Score score;
    Lines lines;
    lines[0] = "0";
    score.update(lines);

    lines[0] = "\t";
    score.update(lines);

    EXPECT_EQ(eventsOn(score, 0), 0U);
    EXPECT_TRUE(score.error(0).empty());
}

// A line whose first character is not space still reads as written.
TEST(ScoreTest, ALineStartingWithContentIsNotBlank)
{
    Score score;
    Lines lines;
    lines[0] = " 0 ";
    score.update(lines);

    EXPECT_EQ(eventsOn(score, 0), 1U);
}

// A same-length change compares differently from one that grows.
// Both are worth reading again.
TEST(ScoreTest, ReadsALineAgainWhateverWayItChanged)
{
    Score score;
    Lines lines;

    lines[0] = "0 3";
    score.update(lines);
    const auto first = score.reparses();

    lines[0] = "0 5";
    score.update(lines);
    EXPECT_EQ(score.reparses(), first + 1);

    lines[0] = "0 5 7";
    score.update(lines);
    EXPECT_EQ(score.reparses(), first + 2);

    lines[0] = "";
    score.update(lines);
    EXPECT_EQ(score.reparses(), first + 3);
}
