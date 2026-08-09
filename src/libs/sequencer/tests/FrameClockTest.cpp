#include <gtest/gtest.h>

#include <chrono>
#include <cstdint>
#include <limits>

#include "antwika/sequencer/FrameClock.hpp"
#include "antwika/sequencer/Rational.hpp"
#include "antwika/sequencer/SequencerError.hpp"

using antwika::sequencer::FrameClock;
using antwika::sequencer::Rational;
using antwika::sequencer::SequencerError;

namespace
{
    using namespace std::chrono_literals;
}

TEST(FrameClockTest, Ctor_RefusesARateOfNothing)
{
    EXPECT_THROW(FrameClock(0, 40ms), SequencerError);
}

TEST(FrameClockTest, Ctor_RefusesATickOfNoTime)
{
    EXPECT_THROW(FrameClock(48000, 0ms), SequencerError);
    EXPECT_THROW(FrameClock(48000, -1ms), SequencerError);
}

TEST(FrameClockTest, Ctor_AcceptsTheShortestTickThatIsStillAnInterval)
{
    const FrameClock clock(48000, 1ms);

    EXPECT_EQ(clock.framesPerTick(), Rational(48));
    EXPECT_EQ(clock.frameAtTick(1), 48U);
}

TEST(FrameClockTest, FrameAtTick_PlacesOnWholeFramesWhenTheyDivide)
{
    const FrameClock clock(48000, 40ms);

    EXPECT_EQ(clock.framesPerTick(), Rational(1920));
    EXPECT_EQ(clock.frameAtTick(0), 0U);
    EXPECT_EQ(clock.frameAtTick(1), 1920U);
    EXPECT_EQ(clock.frameAtTick(25), 48000U);
}

TEST(FrameClockTest, FrameAtTick_PlacesExactlyWhenTheyDoNotDivide)
{
    const FrameClock clock(44100, 3ms);

    EXPECT_EQ(clock.framesPerTick(), Rational(1323, 10));

    EXPECT_EQ(clock.frameAtTick(1), 132U);
    EXPECT_EQ(clock.frameAtTick(2), 264U);
    EXPECT_EQ(clock.frameAtTick(3), 396U);

    EXPECT_EQ(clock.frameAtTick(10), 1323U);
    EXPECT_EQ(clock.frameAtTick(1000), 132300U);
}

TEST(FrameClockTest, FrameAtTick_NeverDriftsHoweverFarAlong)
{
    const FrameClock clock(44100, 3ms);

    EXPECT_EQ(clock.frameAtTick(1000000), 132300000U);
}

TEST(FrameClockTest, FrameAtTick_RefusesATickTooFarAlong)
{
    const FrameClock clock(48000, 40ms);

    const auto beyond = static_cast<std::uint64_t>(
                            std::numeric_limits<std::int64_t>::max())
        + 1U;

    EXPECT_THROW((void)clock.frameAtTick(beyond), SequencerError);
}
