#include <gtest/gtest.h>

#include <chrono>
#include <cstdint>
#include <vector>

#include <antwika/pattern/Controls.hpp>
#include <antwika/pattern/Cycle.hpp>
#include <antwika/pattern/ParamId.hpp>
#include <antwika/pattern/ParamValue.hpp>
#include <antwika/pattern/Pattern.hpp>
#include <antwika/pattern/Patterns.hpp>
#include <antwika/sequencer/fakes/FakeRecordingSink.hpp>
#include <antwika/time/Tick.hpp>

#include "antwika/sequencer/Sequencer.hpp"
#include "antwika/sequencer/FrameClock.hpp"
#include "antwika/sequencer/ISequencerSink.hpp"
#include "antwika/sequencer/Rational.hpp"
#include "antwika/sequencer/SequencerError.hpp"
#include "antwika/sequencer/TempoMap.hpp"

using antwika::pattern::Controls;
using antwika::pattern::Cycle;
using antwika::pattern::fastcat;
using antwika::pattern::ParamId;
using antwika::pattern::ParamValue;
using antwika::pattern::Pattern;
using antwika::pattern::pure;
using antwika::pattern::steady;
using antwika::sequencer::FrameClock;
using antwika::sequencer::ISequencerSink;
using antwika::sequencer::Rational;
using antwika::sequencer::Sequencer;
using antwika::sequencer::SequencerDesc;
using antwika::sequencer::SequencerError;
using antwika::sequencer::TempoMap;
using antwika::sequencer::fakes::FakeRecordingSink;
using antwika::sequencer::fakes::Sounded;

namespace
{
    using namespace std::chrono_literals;

    constexpr ParamId kName{1};

    [[nodiscard]] Controls named(std::int64_t which)
    {
        return Controls(kName, ParamValue(which));
    }

    [[nodiscard]] Pattern note(std::int64_t which)
    {
        return pure(named(which));
    }

    [[nodiscard]] SequencerDesc oneCycleASecond()
    {
        return SequencerDesc{
            .clock = FrameClock(48000, 100ms),
            .tempo = TempoMap(Rational(48000)),
            .lookahead = 1};
    }
}

TEST(SequencerTest, Ctor_RefusesToLookNoTicksAhead)
{
    auto desc = oneCycleASecond();
    desc.lookahead = 0;

    EXPECT_THROW(Sequencer{std::move(desc)}, SequencerError);
}

TEST(SequencerTest, QueriedThrough_StartsAtNothing)
{
    const Sequencer sequencer(oneCycleASecond());

    EXPECT_EQ(sequencer.queriedThrough(), Cycle());
}

TEST(SequencerTest, Advance_SoundsAnEventOnItsMappedFrame)
{
    Sequencer sequencer(oneCycleASecond());
    FakeRecordingSink sink;

    sequencer.advance(0, note(1), sink);

    ASSERT_EQ(sink.triggers.size(), 1U);
    EXPECT_EQ(sink.triggers[0].value, named(1));
    EXPECT_EQ(sink.triggers[0].startFrame, 0U);
    EXPECT_EQ(sink.triggers[0].frames, 48000U);
}

TEST(SequencerTest, Advance_MovesTheWindowOneTickAtATime)
{
    Sequencer sequencer(oneCycleASecond());
    FakeRecordingSink sink;

    sequencer.advance(0, note(1), sink);

    EXPECT_EQ(sequencer.queriedThrough(), Cycle(1, 10));

    sequencer.advance(1, note(1), sink);

    EXPECT_EQ(sequencer.queriedThrough(), Cycle(1, 5));
}

TEST(SequencerTest, Advance_NeverSoundsAHeldNoteTwice)
{
    Sequencer sequencer(oneCycleASecond());
    FakeRecordingSink sink;

    for (antwika::time::Tick tick = 0; tick < 10; ++tick)
    {
        sequencer.advance(tick, note(1), sink);
    }

    ASSERT_EQ(sink.triggers.size(), 1U);
    EXPECT_EQ(sink.triggers[0].startFrame, 0U);
}

TEST(SequencerTest, Retime_MovesEveryNoteAfterTheBoundary)
{
    Sequencer sequencer(oneCycleASecond());
    FakeRecordingSink sink;

    for (antwika::time::Tick tick = 0; tick < 5; ++tick)
    {
        sequencer.advance(tick, note(1), sink);
    }

    sequencer.retime(Cycle(1), Rational(24000));

    for (antwika::time::Tick tick = 5; tick <= 30; ++tick)
    {
        sequencer.advance(tick, note(1), sink);
    }

    ASSERT_EQ(sink.triggers.size(), 6U);

    EXPECT_EQ(sink.triggers[0].startFrame, 0U);
    EXPECT_EQ(sink.triggers[0].frames, 48000U);

    EXPECT_EQ(sink.triggers[1].startFrame, 48000U);
    EXPECT_EQ(sink.triggers[1].frames, 24000U);

    EXPECT_EQ(sink.triggers[2].startFrame, 72000U);
}

TEST(SequencerTest, Retime_RefusesInsideAQueriedWindow)
{
    Sequencer sequencer(oneCycleASecond());
    FakeRecordingSink sink;

    sequencer.advance(0, note(1), sink);

    EXPECT_THROW(
        sequencer.retime(Cycle(1, 20), Rational(24000)),
        SequencerError);
}

TEST(SequencerTest, Retime_AcceptsTheCycleTheWindowStopsAt)
{
    Sequencer sequencer(oneCycleASecond());
    FakeRecordingSink sink;

    sequencer.advance(0, note(1), sink);

    ASSERT_EQ(sequencer.queriedThrough(), Cycle(1, 10));

    sequencer.retime(Cycle(1, 10), Rational(24000));

    sink.triggers.clear();
    sequencer.advance(10, note(1), sink);

    ASSERT_EQ(sink.triggers.size(), 2U);
    EXPECT_EQ(sink.triggers[0].startFrame, 26400U);
    EXPECT_EQ(sink.triggers[1].startFrame, 50400U);
}

TEST(SequencerTest, Advance_SoundsEachEventOnceAsItArrives)
{
    Sequencer sequencer(oneCycleASecond());
    FakeRecordingSink sink;

    const auto beat = fastcat({note(1), note(2)});

    for (antwika::time::Tick tick = 0; tick < 10; ++tick)
    {
        sequencer.advance(tick, beat, sink);
    }

    ASSERT_EQ(sink.triggers.size(), 2U);

    EXPECT_EQ(sink.triggers[0].value, named(1));
    EXPECT_EQ(sink.triggers[0].startFrame, 0U);
    EXPECT_EQ(sink.triggers[0].frames, 24000U);

    EXPECT_EQ(sink.triggers[1].value, named(2));
    EXPECT_EQ(sink.triggers[1].startFrame, 24000U);
    EXPECT_EQ(sink.triggers[1].frames, 24000U);
}

TEST(SequencerTest, Advance_SoundsNothingWhenAskedAgain)
{
    Sequencer sequencer(oneCycleASecond());
    FakeRecordingSink sink;

    sequencer.advance(0, note(1), sink);
    sequencer.advance(0, note(1), sink);

    EXPECT_EQ(sink.triggers.size(), 1U);
}

TEST(SequencerTest, Advance_SoundsNothingGoingBackwards)
{
    Sequencer sequencer(oneCycleASecond());
    FakeRecordingSink sink;

    sequencer.advance(5, note(1), sink);
    const auto reached = sequencer.queriedThrough();

    ASSERT_EQ(reached, Cycle(3, 5));

    sink.triggers.clear();
    sequencer.advance(1, note(1), sink);

    EXPECT_TRUE(sink.triggers.empty());
    EXPECT_EQ(sequencer.queriedThrough(), reached);
}

TEST(SequencerTest, JoinAt_MovesTheWindowToTheJoinedTick)
{
    Sequencer sequencer(oneCycleASecond());

    sequencer.joinAt(20);

    EXPECT_EQ(sequencer.queriedThrough(), Cycle(21, 10));
}

TEST(SequencerTest, Advance_CatchesUpOnEveryCycleSinceTheStart)
{
    Sequencer sequencer(oneCycleASecond());
    FakeRecordingSink sink;

    sequencer.advance(21, note(1), sink);

    EXPECT_EQ(sink.triggers.size(), 3U);
    EXPECT_EQ(sequencer.queriedThrough(), Cycle(11, 5));
}

TEST(SequencerTest, JoinAt_LeavesThePastAlone)
{
    Sequencer sequencer(oneCycleASecond());
    FakeRecordingSink sink;

    sequencer.joinAt(20);
    sequencer.advance(21, note(1), sink);

    EXPECT_TRUE(sink.triggers.empty());
    EXPECT_EQ(sequencer.queriedThrough(), Cycle(11, 5));
}

TEST(SequencerTest, JoinAt_SoundsWhatIsAdvancedOverAfterwards)
{
    Sequencer sequencer(oneCycleASecond());
    FakeRecordingSink sink;

    sequencer.joinAt(20);

    for (antwika::time::Tick tick = 21; tick < 31; ++tick)
    {
        sequencer.advance(tick, note(1), sink);
    }

    ASSERT_EQ(sink.triggers.size(), 1U);
    EXPECT_EQ(sink.triggers[0].startFrame, 144000U);
}

TEST(SequencerTest, JoinAt_LeavesTheWindowWhenGoingBackwards)
{
    Sequencer sequencer(oneCycleASecond());
    FakeRecordingSink sink;

    sequencer.advance(20, note(1), sink);
    const auto reached = sequencer.queriedThrough();

    ASSERT_EQ(reached, Cycle(21, 10));

    sequencer.joinAt(1);
    EXPECT_EQ(sequencer.queriedThrough(), reached);

    sequencer.joinAt(20);
    EXPECT_EQ(sequencer.queriedThrough(), reached);

    sink.triggers.clear();
    sequencer.advance(20, note(1), sink);

    EXPECT_TRUE(sink.triggers.empty());
}

TEST(SequencerTest, Advance_NeverSoundsAContinuousValue)
{
    Sequencer sequencer(oneCycleASecond());
    FakeRecordingSink sink;

    for (antwika::time::Tick tick = 0; tick < 20; ++tick)
    {
        sequencer.advance(tick, steady(named(9)), sink);
    }

    EXPECT_TRUE(sink.triggers.empty());
}

TEST(SequencerTest, Advance_LooksFurtherAheadWhenAskedTo)
{
    auto desc = oneCycleASecond();
    desc.lookahead = 10;

    Sequencer sequencer(std::move(desc));
    FakeRecordingSink sink;

    sequencer.advance(0, fastcat({note(1), note(2)}), sink);

    EXPECT_EQ(sink.triggers.size(), 2U);
    EXPECT_EQ(sequencer.queriedThrough(), Cycle(1));
}

TEST(SequencerTest, Advance_FollowsATempoChange)
{
    TempoMap tempo(Rational(48000));
    tempo.addSegment(Cycle(1), Rational(24000));

    Sequencer sequencer(
        SequencerDesc{
            .clock = FrameClock(48000, 100ms),
            .tempo = std::move(tempo),
            .lookahead = 30});

    FakeRecordingSink sink;
    sequencer.advance(0, note(1), sink);

    ASSERT_EQ(sink.triggers.size(), 5U);

    EXPECT_EQ(sink.triggers[0].startFrame, 0U);
    EXPECT_EQ(sink.triggers[0].frames, 48000U);

    EXPECT_EQ(sink.triggers[1].startFrame, 48000U);
    EXPECT_EQ(sink.triggers[1].frames, 24000U);

    EXPECT_EQ(sink.triggers[2].startFrame, 72000U);
    EXPECT_EQ(sink.triggers[4].startFrame, 120000U);
}

TEST(SequencerTest, Advance_SoundsAnEventCarryingNothing)
{
    Sequencer sequencer(oneCycleASecond());
    FakeRecordingSink sink;

    sequencer.advance(0, pure(Controls{}), sink);

    ASSERT_EQ(sink.triggers.size(), 1U);
    EXPECT_TRUE(sink.triggers[0].value.empty());
    EXPECT_EQ(sink.triggers[0].frames, 48000U);
}
