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

    class FrameClock final
    {
    public:
        FrameClock(SampleRate rate, std::chrono::milliseconds interval);

        [[nodiscard]] FrameIndex frameAtTick(time::Tick tick) const;

        [[nodiscard]] Rational framesPerTick() const noexcept;

    private:
        Rational perTick;
    };

}
