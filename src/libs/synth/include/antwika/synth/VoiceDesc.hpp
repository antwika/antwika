#pragma once

#include <cstdint>

#include <antwika/sound/Frames.hpp>

#include "antwika/synth/Adsr.hpp"
#include "antwika/synth/Filter.hpp"
#include "antwika/synth/Waveshape.hpp"

namespace antwika::synth
{

    using antwika::sound::FrameCount;

    struct VoiceDesc final
    {
        Waveshape shape = Waveshape::Sine;

        double frequency = 440.0;

        double frequencySlide = 0.0;

        Adsr envelope{};

        FrameCount hold = 0;

        FilterDesc filter{};

        float gain = 1.0F;

        float pan = 0.0F;

        double vibratoHertz = 0.0;

        double vibratoDepth = 0.0;

        double arpeggioRatio = 1.0;

        FrameCount arpeggioPeriod = 0;

        std::uint64_t seed = 0;

        [[nodiscard]] FrameCount totalFrames() const noexcept;

        [[nodiscard]] bool operator==(const VoiceDesc &other) const
            = default;
    };

}
