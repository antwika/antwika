#include "antwika/sequencer/TempoMap.hpp"

#include <cstddef>

#include <gtest/gtest.h>

#include <antwika/pattern/Cycle.hpp>

#include "antwika/sequencer/Rational.hpp"
#include "antwika/sequencer/SequencerError.hpp"

using antwika::pattern::Cycle;
using antwika::sequencer::Rational;
using antwika::sequencer::SequencerError;
using antwika::sequencer::TempoMap;

TEST(TempoMapTest, RefusesATempoThatWouldNeverAdvance)
{
    EXPECT_THROW(TempoMap{Rational()}, SequencerError);
    EXPECT_THROW(TempoMap{Rational(-1)}, SequencerError);
}

TEST(TempoMapTest, PlacesCyclesAtOneTempo)
{
    const TempoMap tempo(Rational(48000));

    EXPECT_EQ(tempo.segmentCount(), 1U);
    EXPECT_EQ(tempo.framesAt(Cycle()), 0U);
    EXPECT_EQ(tempo.framesAt(Cycle(1)), 48000U);
    EXPECT_EQ(tempo.framesAt(Cycle(1, 2)), 24000U);
    EXPECT_EQ(tempo.framesAt(Cycle(3, 4)), 36000U);
}

// Exact inverses, which is what makes a note land where the score said.
TEST(TempoMapTest, TurnsFramesBackIntoCycles)
{
    const TempoMap tempo(Rational(48000));

    EXPECT_EQ(tempo.cycleAt(0), Cycle());
    EXPECT_EQ(tempo.cycleAt(48000), Cycle(1));
    EXPECT_EQ(tempo.cycleAt(24000), Cycle(1, 2));
    EXPECT_EQ(tempo.cycleAt(1), Cycle(1, 48000));
}

// A cycle that is not a whole number of frames is still exact here.
// The flooring happens once, at the very end.
TEST(TempoMapTest, HoldsATempoThatIsNotAWholeNumberOfFrames)
{
    const TempoMap tempo(Rational(48000, 7));

    EXPECT_EQ(tempo.framesAt(Cycle(7)), 48000U);
    EXPECT_EQ(tempo.framesAt(Cycle(1)), 6857U);
    EXPECT_EQ(tempo.cycleAt(48000), Cycle(7));
}

// The defaulted comparison must consult every field, not stop early.
TEST(TempoMapTest, SegmentEqualityComparesEveryField)
{
    const TempoMap::Segment base{
        .startCycle = Cycle(1),
        .startFrame = 100,
        .framesPerCycle = Rational(2)};

    auto moved = base;
    moved.startCycle = Cycle(2);
    EXPECT_NE(base, moved);

    auto shifted = base;
    shifted.startFrame = 200;
    EXPECT_NE(base, shifted);

    auto paced = base;
    paced.framesPerCycle = Rational(3);
    EXPECT_NE(base, paced);
}

TEST(TempoMapTest, ChangesTempoFromACycleOnwards)
{
    TempoMap tempo(Rational(48000));
    tempo.addSegment(Cycle(2), Rational(24000));

    EXPECT_EQ(tempo.segmentCount(), 2U);

    // The first two cycles run at the first tempo.
    EXPECT_EQ(tempo.framesAt(Cycle(2)), 96000U);

    // The third runs at the second, so it is half as long.
    EXPECT_EQ(tempo.framesAt(Cycle(3)), 120000U);
    EXPECT_EQ(tempo.framesAt(Cycle(4)), 144000U);
}

TEST(TempoMapTest, TurnsFramesBackIntoCyclesAcrossASegment)
{
    TempoMap tempo(Rational(48000));
    tempo.addSegment(Cycle(2), Rational(24000));

    // Before the second segment starts, so the search stops early.
    EXPECT_EQ(tempo.cycleAt(24000), Cycle(1, 2));

    EXPECT_EQ(tempo.cycleAt(96000), Cycle(2));
    EXPECT_EQ(tempo.cycleAt(120000), Cycle(3));
    EXPECT_EQ(tempo.cycleAt(108000), Cycle(5, 2));
}

TEST(TempoMapTest, RefusesASegmentThatDoesNotComeAfterTheLast)
{
    TempoMap tempo(Rational(48000));
    tempo.addSegment(Cycle(2), Rational(24000));

    EXPECT_THROW(
        tempo.addSegment(Cycle(2), Rational(12000)), SequencerError);

    EXPECT_THROW(
        tempo.addSegment(Cycle(1), Rational(12000)), SequencerError);

    EXPECT_THROW(
        tempo.addSegment(Cycle(4), Rational()), SequencerError);
}

// A pattern shifted early asks about positions before cycle zero.
// It gets a sensible answer rather than a refusal.
TEST(TempoMapTest, ExtrapolatesBeforeTheFirstSegment)
{
    const TempoMap tempo(Rational(48000));

    EXPECT_EQ(tempo.cycleAt(0), Cycle());
    EXPECT_EQ(tempo.framesAt(Cycle(-1, 4)), 0U);
}

TEST(TempoMapTest, ClampsAPositionBeforeTheVeryFirstFrame)
{
    const TempoMap tempo(Rational(48000));

    EXPECT_EQ(tempo.framesAt(Cycle(-5)), 0U);
}

// The table a caller reads back grows with every segment added.
TEST(TempoMapTest, ReportsItsSegmentsInOrder)
{
    TempoMap tempo(Rational(48000));

    ASSERT_EQ(tempo.segments().size(), 1U);
    EXPECT_EQ(tempo.segments().front().startCycle, Cycle());
    EXPECT_EQ(tempo.segments().front().startFrame, 0U);
    EXPECT_EQ(tempo.segments().front().framesPerCycle, Rational(48000));

    tempo.addSegment(Cycle(2), Rational(24000));

    ASSERT_EQ(tempo.segments().size(), 2U);
    EXPECT_EQ(tempo.segments().back().startCycle, Cycle(2));
    EXPECT_EQ(tempo.segments().back().startFrame, 96000U);
    EXPECT_EQ(tempo.segments().back().framesPerCycle, Rational(24000));
}

// A dump rebuilds a map from its table, so the replay must be exact.
// Each startFrame is derived from the segments before it alone.
TEST(TempoMapTest, ReplayingItsSegmentsReproducesAnEqualMap)
{
    TempoMap tempo(Rational(48000));
    tempo.addSegment(Cycle(2), Rational(24000));
    tempo.addSegment(Cycle(4), Rational(96000));

    const auto &table = tempo.segments();

    TempoMap replayed(table.front().framesPerCycle);

    for (std::size_t index = 1; index < table.size(); ++index)
    {
        replayed.addSegment(
            table[index].startCycle, table[index].framesPerCycle);
    }

    // startFrame included: the replay recomputed every one of them.
    EXPECT_EQ(replayed.segments(), table);
}
