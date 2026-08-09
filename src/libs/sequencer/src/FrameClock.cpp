#include "antwika/sequencer/FrameClock.hpp"

#include <chrono>
#include <cstdint>
#include <limits>
#include <string>

#include <antwika/time/Tick.hpp>

#include "antwika/sequencer/Rational.hpp"
#include "antwika/sequencer/SequencerError.hpp"

namespace antwika::sequencer
{

    namespace
    {
        constexpr std::int64_t kMillisecondsPerSecond = 1000;
    }

    FrameClock::FrameClock(
        SampleRate rate, std::chrono::milliseconds interval)
        : perTick(0)
    {
        if (rate == 0)
        {
            throw SequencerError(
                "antwika::sequencer: a clock at no frames a second "
                "could never place a tick");
        }

        if (interval.count() <= 0)
        {
            throw SequencerError(
                "antwika::sequencer: a tick lasting "
                + std::to_string(interval.count())
                + " ms is not an interval");
        }

        perTick = Rational(
            static_cast<std::int64_t>(rate) * interval.count(),
            kMillisecondsPerSecond);
    }

    FrameIndex FrameClock::frameAtTick(time::Tick tick) const
    {
        constexpr auto kMostTicks = static_cast<time::Tick>(
            std::numeric_limits<std::int64_t>::max());

        if (tick > kMostTicks)
        {
            throw SequencerError(
                "antwika::sequencer: tick " + std::to_string(tick)
                + " is too far along to place on a timeline");
        }

        const auto at =
            Rational(static_cast<std::int64_t>(tick)) * perTick;

        return static_cast<FrameIndex>(at.floorCycle());
    }

    Rational FrameClock::framesPerTick() const noexcept
    {
        return perTick;
    }

}
