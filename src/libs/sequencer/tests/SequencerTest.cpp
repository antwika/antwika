#include "antwika/sequencer/Sequencer.hpp"

#include <chrono>
#include <cstdint>
#include <vector>

#include <gtest/gtest.h>

#include <antwika/pattern/Controls.hpp>
#include <antwika/pattern/Cycle.hpp>
#include <antwika/pattern/ParamId.hpp>
#include <antwika/pattern/ParamValue.hpp>
#include <antwika/pattern/Pattern.hpp>
#include <antwika/pattern/Patterns.hpp>
#include <antwika/time/Tick.hpp>

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

    // Keeps its own copy.
    // That is the sink's decision rather than the sequencer's.
    struct Sounded
    {
        Controls value;
        antwika::sound::FrameIndex startFrame = 0;
        antwika::sound::FrameCount frames = 0;
    };

    class RecordingSink final : public ISequencerSink
    {
    public:
        void trigger(
            const Controls &value,
            antwika::sound::FrameIndex startFrame,
            antwika::sound::FrameCount frames) override
        {
            triggers.push_back(
                Sounded{
                    .value = value,
                    .startFrame = startFrame,
                    .frames = frames});
        }

        std::vector<Sounded> triggers;
    };

    // One cycle a second, and a tick every tenth of a second.
    // Looking one tick ahead.
    [[nodiscard]] SequencerDesc oneCycleASecond()
    {
        return SequencerDesc{
            .clock = FrameClock(48000, 100ms),
            .tempo = TempoMap(Rational(48000)),
            .lookahead = 1};
    }
} // namespace

TEST(SequencerTest, RefusesToLookNoTicksAhead)
{
    auto desc = oneCycleASecond();
    desc.lookahead = 0;

    EXPECT_THROW(Sequencer{std::move(desc)}, SequencerError);
}

TEST(SequencerTest, StartsHavingAskedAboutNothing)
{
    const Sequencer sequencer(oneCycleASecond());

    EXPECT_EQ(sequencer.queriedThrough(), Cycle());
}

TEST(SequencerTest, SoundsAnEventOnTheFrameItsPositionMapsTo)
{
    Sequencer sequencer(oneCycleASecond());
    RecordingSink sink;

    sequencer.advance(0, note(1), sink);

    ASSERT_EQ(sink.triggers.size(), 1U);
    EXPECT_EQ(sink.triggers[0].value, named(1));
    EXPECT_EQ(sink.triggers[0].startFrame, 0U);
    EXPECT_EQ(sink.triggers[0].frames, 48000U);
}

TEST(SequencerTest, AdvancesItsWindowByOneTickAtATime)
{
    Sequencer sequencer(oneCycleASecond());
    RecordingSink sink;

    sequencer.advance(0, note(1), sink);

    // A tick is a tenth of a second, and a cycle is a second.
    EXPECT_EQ(sequencer.queriedThrough(), Cycle(1, 10));

    sequencer.advance(1, note(1), sink);

    EXPECT_EQ(sequencer.queriedThrough(), Cycle(1, 5));
}

// The one line the whole library turns on.
// A held note produces a fragment in every window it spans.
// Sounding those would restart it at every boundary.
TEST(SequencerTest, NeverSoundsAHeldNoteTwice)
{
    Sequencer sequencer(oneCycleASecond());
    RecordingSink sink;

    for (antwika::time::Tick tick = 0; tick < 10; ++tick)
    {
        sequencer.advance(tick, note(1), sink);
    }

    // Ten ticks is one cycle, and one cycle of pure() is one event.
    ASSERT_EQ(sink.triggers.size(), 1U);
    EXPECT_EQ(sink.triggers[0].startFrame, 0U);
}

TEST(SequencerTest, SoundsEachEventOfASequenceOnceAsItArrives)
{
    Sequencer sequencer(oneCycleASecond());
    RecordingSink sink;

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

// The window only ever moves forward.
// A tick asked about twice sounds nothing the second time.
TEST(SequencerTest, AskingAgainForATickSoundsNothing)
{
    Sequencer sequencer(oneCycleASecond());
    RecordingSink sink;

    sequencer.advance(0, note(1), sink);
    sequencer.advance(0, note(1), sink);

    EXPECT_EQ(sink.triggers.size(), 1U);
}

TEST(SequencerTest, GoingBackwardsSoundsNothing)
{
    Sequencer sequencer(oneCycleASecond());
    RecordingSink sink;

    sequencer.advance(5, note(1), sink);
    const auto reached = sequencer.queriedThrough();

    sink.triggers.clear();
    sequencer.advance(1, note(1), sink);

    EXPECT_TRUE(sink.triggers.empty());
    EXPECT_EQ(sequencer.queriedThrough(), reached);
}

// A voice that was not there a moment ago has no past to play.
// Joining says so, and says it without querying anything.
TEST(SequencerTest, JoiningSoundsNothingItself)
{
    Sequencer sequencer(oneCycleASecond());
    RecordingSink sink;

    sequencer.joinAt(20);

    EXPECT_TRUE(sink.triggers.empty());

    // Ten ticks to a cycle, and it looks one tick further.
    EXPECT_EQ(sequencer.queriedThrough(), Cycle(21, 10));
}

// A sequencer built fresh sounds every cycle since the run began.
// One that joined sounds that tick's window and nothing else.
TEST(SequencerTest, JoiningLeavesThePastToWhoeverWasThereForIt)
{
    RecordingSink catchingUp;
    Sequencer fresh(oneCycleASecond());

    fresh.advance(21, note(1), catchingUp);

    RecordingSink joinedSink;
    Sequencer joined(oneCycleASecond());

    joined.joinAt(20);
    joined.advance(21, note(1), joinedSink);

    // Two whole cycles of history, sounded all at once.
    EXPECT_EQ(catchingUp.triggers.size(), 3U);

    // A tenth of a cycle, holding no onset at all.
    EXPECT_TRUE(joinedSink.triggers.empty());
    EXPECT_EQ(joined.queriedThrough(), fresh.queriedThrough());
}

TEST(SequencerTest, AJoinedSequencerSoundsWhatItIsAdvancedOver)
{
    Sequencer sequencer(oneCycleASecond());
    RecordingSink sink;

    sequencer.joinAt(20);

    for (antwika::time::Tick tick = 21; tick < 31; ++tick)
    {
        sequencer.advance(tick, note(1), sink);
    }

    // One cycle of window, and pure() begins once a cycle.
    ASSERT_EQ(sink.triggers.size(), 1U);
    EXPECT_EQ(sink.triggers[0].startFrame, 144000U);
}

// Moving backwards is not expressible.
TEST(SequencerTest, JoiningBackwardsLeavesTheWindowWhereItIs)
{
    Sequencer sequencer(oneCycleASecond());
    RecordingSink sink;

    sequencer.advance(20, note(1), sink);
    const auto reached = sequencer.queriedThrough();

    sequencer.joinAt(1);
    EXPECT_EQ(sequencer.queriedThrough(), reached);

    // And joining where it already stands moves nothing either.
    sequencer.joinAt(20);
    EXPECT_EQ(sequencer.queriedThrough(), reached);

    sink.triggers.clear();
    sequencer.advance(20, note(1), sink);

    EXPECT_TRUE(sink.triggers.empty());
}

// A continuous value never begins, so it is never sounded.
TEST(SequencerTest, NeverSoundsAContinuousValue)
{
    Sequencer sequencer(oneCycleASecond());
    RecordingSink sink;

    for (antwika::time::Tick tick = 0; tick < 20; ++tick)
    {
        sequencer.advance(tick, steady(named(9)), sink);
    }

    EXPECT_TRUE(sink.triggers.empty());
}

TEST(SequencerTest, LooksFurtherAheadWhenAskedTo)
{
    auto desc = oneCycleASecond();
    desc.lookahead = 10;

    Sequencer sequencer(std::move(desc));
    RecordingSink sink;

    // One tick, but ten ticks of lookahead.
    // So a whole cycle is decided at once.
    sequencer.advance(0, fastcat({note(1), note(2)}), sink);

    EXPECT_EQ(sink.triggers.size(), 2U);
    EXPECT_EQ(sequencer.queriedThrough(), Cycle(1));
}

TEST(SequencerTest, FollowsATempoChange)
{
    TempoMap tempo(Rational(48000));
    tempo.addSegment(Cycle(1), Rational(24000));

    Sequencer sequencer(
        SequencerDesc{
            .clock = FrameClock(48000, 100ms),
            .tempo = std::move(tempo),
            .lookahead = 30});

    RecordingSink sink;
    sequencer.advance(0, note(1), sink);

    // Three seconds of frames, and the tempo doubles after cycle one.
    // So five cycles fit inside them.
    ASSERT_EQ(sink.triggers.size(), 5U);

    // The first cycle is a second long, the ones after it half of one.
    EXPECT_EQ(sink.triggers[0].startFrame, 0U);
    EXPECT_EQ(sink.triggers[0].frames, 48000U);

    EXPECT_EQ(sink.triggers[1].startFrame, 48000U);
    EXPECT_EQ(sink.triggers[1].frames, 24000U);

    EXPECT_EQ(sink.triggers[2].startFrame, 72000U);
    EXPECT_EQ(sink.triggers[4].startFrame, 120000U);
}


// An event is free to carry nothing at all.
// What a control means is the application's business, not this one's.
TEST(SequencerTest, SoundsAnEventCarryingNothing)
{
    Sequencer sequencer(oneCycleASecond());
    RecordingSink sink;

    sequencer.advance(0, pure(Controls{}), sink);

    ASSERT_EQ(sink.triggers.size(), 1U);
    EXPECT_TRUE(sink.triggers[0].value.empty());
    EXPECT_EQ(sink.triggers[0].frames, 48000U);
}
