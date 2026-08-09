#include <gtest/gtest.h>

#include <cstddef>

#include <antwika/pattern/Cycle.hpp>

#include "antwika/sequencer/TempoMap.hpp"
#include "antwika/sequencer/Rational.hpp"
#include "antwika/sequencer/SequencerError.hpp"

using antwika::pattern::Cycle;
using antwika::sequencer::Rational;
using antwika::sequencer::SequencerError;
using antwika::sequencer::TempoMap;

TEST(TempoMapTest, Ctor_RefusesATempoThatNeverAdvances)
{
    EXPECT_THROW(TempoMap{Rational()}, SequencerError);
    EXPECT_THROW(TempoMap{Rational(-1)}, SequencerError);
}

TEST(TempoMapTest, FramesAt_PlacesCyclesAtOneTempo)
{
    const TempoMap tempo(Rational(48000));

    EXPECT_EQ(tempo.segmentCount(), 1U);
    EXPECT_EQ(tempo.framesAt(Cycle()), 0U);
    EXPECT_EQ(tempo.framesAt(Cycle(1)), 48000U);
    EXPECT_EQ(tempo.framesAt(Cycle(1, 2)), 24000U);
    EXPECT_EQ(tempo.framesAt(Cycle(3, 4)), 36000U);
}

TEST(TempoMapTest, CycleAt_TurnsFramesBackIntoCycles)
{
    const TempoMap tempo(Rational(48000));

    EXPECT_EQ(tempo.cycleAt(0), Cycle());
    EXPECT_EQ(tempo.cycleAt(48000), Cycle(1));
    EXPECT_EQ(tempo.cycleAt(24000), Cycle(1, 2));
    EXPECT_EQ(tempo.cycleAt(1), Cycle(1, 48000));
}

TEST(TempoMapTest, FramesAt_HoldsAFractionalFrameTempo)
{
    const TempoMap tempo(Rational(48000, 7));

    EXPECT_EQ(tempo.framesAt(Cycle(7)), 48000U);
    EXPECT_EQ(tempo.framesAt(Cycle(1)), 6857U);
    EXPECT_EQ(tempo.cycleAt(48000), Cycle(7));
}

TEST(TempoMapTest, OperatorEquals_ComparesEverySegmentField)
{
    const TempoMap::Segment base{
        .startCycle = Cycle(1),
        .startFrame = 100,
        .framesPerCycle = Rational(2)};

    const auto twin = base;
    EXPECT_EQ(base, twin);

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

TEST(TempoMapTest, AddSegment_ChangesTempoFromACycleOnwards)
{
    TempoMap tempo(Rational(48000));
    tempo.addSegment(Cycle(2), Rational(24000));

    EXPECT_EQ(tempo.segmentCount(), 2U);

    EXPECT_EQ(tempo.framesAt(Cycle(2)), 96000U);

    EXPECT_EQ(tempo.framesAt(Cycle(3)), 120000U);
    EXPECT_EQ(tempo.framesAt(Cycle(4)), 144000U);
}

TEST(TempoMapTest, CycleAt_TurnsFramesBackAcrossASegment)
{
    TempoMap tempo(Rational(48000));
    tempo.addSegment(Cycle(2), Rational(24000));

    EXPECT_EQ(tempo.cycleAt(24000), Cycle(1, 2));

    EXPECT_EQ(tempo.cycleAt(96000), Cycle(2));
    EXPECT_EQ(tempo.cycleAt(120000), Cycle(3));
    EXPECT_EQ(tempo.cycleAt(108000), Cycle(5, 2));
}

TEST(TempoMapTest, AddSegment_RefusesOneNotAfterTheLast)
{
    TempoMap tempo(Rational(48000));
    tempo.addSegment(Cycle(2), Rational(24000));

    EXPECT_THROW(
        tempo.addSegment(Cycle(2), Rational(12000)), SequencerError);

    EXPECT_THROW(
        tempo.addSegment(Cycle(1), Rational(12000)), SequencerError);
}

TEST(TempoMapTest, AddSegment_RefusesATempoThatNeverAdvances)
{
    TempoMap tempo(Rational(48000));
    tempo.addSegment(Cycle(2), Rational(24000));

    EXPECT_THROW(
        tempo.addSegment(Cycle(4), Rational()), SequencerError);

    EXPECT_THROW(
        tempo.addSegment(Cycle(4), Rational(-1)), SequencerError);

    EXPECT_EQ(tempo.segmentCount(), 2U);
}

TEST(TempoMapTest, FramesAt_ClampsAFractionOfACycleBeforeTheStart)
{
    const TempoMap tempo(Rational(48000));

    EXPECT_EQ(tempo.framesAt(Cycle(-1, 4)), 0U);
}

TEST(TempoMapTest, FramesAt_ClampsBeforeTheVeryFirstFrame)
{
    const TempoMap tempo(Rational(48000));

    EXPECT_EQ(tempo.framesAt(Cycle(-5)), 0U);
}

TEST(TempoMapTest, CycleAt_ReadsARoundedBoundaryFrameAsTheSegmentStart)
{
    TempoMap tempo(Rational(48000, 7));
    tempo.addSegment(Cycle(1), Rational(24000));

    ASSERT_EQ(tempo.segments().back().startFrame, 6857U);

    EXPECT_EQ(tempo.cycleAt(6857), Cycle(1));
    EXPECT_EQ(tempo.cycleAt(6856), Cycle(5999, 6000));
}

TEST(TempoMapTest, Segments_AreReportedInOrder)
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

TEST(TempoMapTest, Segments_ReplayIntoAnEqualMap)
{
    TempoMap tempo(Rational(48000));
    tempo.addSegment(Cycle(2), Rational(24000));
    tempo.addSegment(Cycle(4), Rational(96000));

    const auto &table = tempo.segments();

    ASSERT_EQ(table.size(), 3U);

    TempoMap replayed(table.front().framesPerCycle);

    for (std::size_t index = 1; index < table.size(); ++index)
    {
        replayed.addSegment(
            table[index].startCycle, table[index].framesPerCycle);
    }

    EXPECT_EQ(replayed.segments(), table);
}
