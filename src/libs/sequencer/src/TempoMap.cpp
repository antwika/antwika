#include "antwika/sequencer/TempoMap.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include <antwika/pattern/Cycle.hpp>
#include <antwika/sound/Frames.hpp>

#include "antwika/sequencer/Rational.hpp"
#include "antwika/sequencer/SequencerError.hpp"

namespace antwika::sequencer
{

    namespace
    {
        [[nodiscard]] std::string describe(const Rational &rate)
        {
            return std::to_string(rate.numerator()) + "/"
                + std::to_string(rate.denominator());
        }

        void refuseAStandingTempo(const Rational &framesPerCycle)
        {
            if (framesPerCycle <= Rational())
            {
                throw SequencerError(
                    "antwika::sequencer: a cycle lasting "
                    + describe(framesPerCycle)
                    + " frames would never advance");
            }
        }
    } // namespace

    TempoMap::TempoMap(Rational framesPerCycle)
    {
        refuseAStandingTempo(framesPerCycle);

        segments.push_back(
            Segment{
                .startCycle = Cycle(),
                .startFrame = 0,
                .framesPerCycle = framesPerCycle});
    }

    void TempoMap::addSegment(Cycle startCycle, Rational framesPerCycle)
    {
        refuseAStandingTempo(framesPerCycle);

        if (startCycle <= segments.back().startCycle)
        {
            throw SequencerError(
                "antwika::sequencer: a tempo segment at cycle "
                + std::to_string(startCycle.numerator()) + "/"
                + std::to_string(startCycle.denominator())
                + " does not come after the one before it");
        }

        // Worked out before the segment is added.
        // So the previous tempo decides where this one begins.
        const auto startFrame = framesAt(startCycle);

        segments.push_back(
            Segment{
                .startCycle = startCycle,
                .startFrame = startFrame,
                .framesPerCycle = framesPerCycle});
    }

    std::size_t TempoMap::segmentCount() const noexcept
    {
        return segments.size();
    }

    const TempoMap::Segment &TempoMap::segmentFor(
        Cycle at) const noexcept
    {
        std::size_t chosen = 0;

        for (std::size_t index = 1; index < segments.size(); ++index)
        {
            if (segments[index].startCycle > at)
            {
                break;
            }

            chosen = index;
        }

        return segments[chosen];
    }

    const TempoMap::Segment &TempoMap::segmentFor(
        FrameIndex frame) const noexcept
    {
        std::size_t chosen = 0;

        for (std::size_t index = 1; index < segments.size(); ++index)
        {
            if (segments[index].startFrame > frame)
            {
                break;
            }

            chosen = index;
        }

        return segments[chosen];
    }

    FrameIndex TempoMap::framesAt(Cycle at) const
    {
        const auto &segment = segmentFor(at);

        const auto into =
            (at - segment.startCycle) * segment.framesPerCycle;

        const auto frames =
            static_cast<std::int64_t>(segment.startFrame)
            + into.floorCycle();

        // A position well before the first segment maps below zero.
        // There is no frame before the very first one.
        return frames < 0 ? 0 : static_cast<FrameIndex>(frames);
    }

    Cycle TempoMap::cycleAt(FrameIndex frame) const
    {
        const auto &segment = segmentFor(frame);

        const auto into = Rational(
            static_cast<std::int64_t>(frame)
            - static_cast<std::int64_t>(segment.startFrame));

        return segment.startCycle + into / segment.framesPerCycle;
    }

} // namespace antwika::sequencer
