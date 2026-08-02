#include "antwika/sequencer/FrameClock.hpp"

#include <chrono>
#include <cstdint>
#include <limits>

#include <gtest/gtest.h>

#include "antwika/sequencer/Rational.hpp"
#include "antwika/sequencer/SequencerError.hpp"

using antwika::sequencer::FrameClock;
using antwika::sequencer::Rational;
using antwika::sequencer::SequencerError;

namespace
{
    using namespace std::chrono_literals;
} // namespace

TEST(FrameClockTest, RefusesARateOfNothing)
{
    EXPECT_THROW(FrameClock(0, 40ms), SequencerError);
}

TEST(FrameClockTest, RefusesATickOfNoTime)
{
    EXPECT_THROW(FrameClock(48000, 0ms), SequencerError);
    EXPECT_THROW(FrameClock(48000, -1ms), SequencerError);
}

TEST(FrameClockTest, PlacesTicksOnWholeFramesWhenTheyDivide)
{
    const FrameClock clock(48000, 40ms);

    EXPECT_EQ(clock.framesPerTick(), Rational(1920));
    EXPECT_EQ(clock.frameAtTick(0), 0U);
    EXPECT_EQ(clock.frameAtTick(1), 1920U);
    EXPECT_EQ(clock.frameAtTick(25), 48000U);
}

// The answer to "the rate does not divide evenly into the tick rate".
// The residue lives in the expression, so nothing accumulates.
TEST(FrameClockTest, PlacesTicksExactlyWhenTheyDoNotDivide)
{
    const FrameClock clock(44100, 3ms);

    EXPECT_EQ(clock.framesPerTick(), Rational(1323, 10));

    EXPECT_EQ(clock.frameAtTick(1), 132U);
    EXPECT_EQ(clock.frameAtTick(2), 264U);
    EXPECT_EQ(clock.frameAtTick(3), 396U);

    // Ten ticks land exactly, however the nine before them floored.
    EXPECT_EQ(clock.frameAtTick(10), 1323U);
    EXPECT_EQ(clock.frameAtTick(1000), 132300U);
}

// A running total would drift here.
// Each tick is worked out from itself alone, so none of them can.
TEST(FrameClockTest, NeverDriftsHoweverFarAlongItIsAsked)
{
    const FrameClock clock(44100, 3ms);

    EXPECT_EQ(clock.frameAtTick(1000000), 132300000U);
}

TEST(FrameClockTest, RefusesATickTooFarAlongToPlace)
{
    const FrameClock clock(48000, 40ms);

    const auto beyond = static_cast<std::uint64_t>(
                            std::numeric_limits<std::int64_t>::max())
        + 1U;

    EXPECT_THROW((void)clock.frameAtTick(beyond), SequencerError);
}
