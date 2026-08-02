#pragma once

#include <chrono>

#include <antwika/sound/Frames.hpp>
#include <antwika/sound/WaveFormat.hpp>
#include <antwika/time/Tick.hpp>

#include "antwika/sequencer/Rational.hpp"

namespace antwika::sequencer
{

    using antwika::sound::FrameIndex;
    using antwika::sound::SampleRate;

    /**
     * @brief Turns a tick into the frame it begins on.
     *
     * **The residue lives in the expression, never in a running
     * variable.**
     * Frames per tick is held as an exact fraction and the frame for a
     * tick is worked out from that tick alone, so nothing accumulates
     * and nothing can lose count.
     *
     * That is the whole answer to "the sample rate does not divide
     * evenly into the tick rate": it does not need to.
     * 48000 over 25 ticks a second is 1920 exactly.
     * 48000 at a 41 ms tick alternates 1968 and 1969 forever without
     * ever drifting.
     */
    class FrameClock final
    {
    public:
        /**
         * @brief Build a clock from a rate and a tick interval.
         * @param rate How many frames a second.
         * @param interval How long one tick lasts.
         * @throws SequencerError If the rate is zero, or the interval is
         * zero or negative.
         */
        FrameClock(SampleRate rate, std::chrono::milliseconds interval);

        /**
         * @brief Get the frame a tick begins on.
         * @param tick Which tick.
         * @return The frame, floored.
         * @throws SequencerError If the tick is too large to place.
         * @throws antwika::pattern::PatternError If the exact
         * arithmetic will not fit.
         */
        [[nodiscard]] FrameIndex frameAtTick(time::Tick tick) const;

        /**
         * @brief Get how many frames one tick spans.
         * @return The count, as an exact fraction.
         */
        [[nodiscard]] Rational framesPerTick() const noexcept;

    private:
        Rational perTick;
    };

} // namespace antwika::sequencer
