#pragma once

#include <cstddef>
#include <vector>

#include <antwika/sequencer/Rational.hpp>
#include <antwika/sound/WaveFormat.hpp>

#include "antwika/music_editor/Score.hpp"

namespace antwika::music_editor
{

    inline constexpr std::size_t kWaveImageColumns = 512;

    struct WaveImage final
    {
        std::vector<float> low{};

        std::vector<float> high{};

        [[nodiscard]] bool operator==(const WaveImage &other) const
            = default;
    };

    struct WaveRenderDesc final
    {
        sound::SampleRate rate = sound::kDefaultSampleRate;

        sequencer::Rational framesPerCycle{};

        [[nodiscard]] bool operator==(const WaveRenderDesc &other) const
            = default;
    };

    [[nodiscard]] WaveImage renderWaveImage(
        const Waveform &wave,
        const WaveRenderDesc &desc,
        sequencer::Rational speed,
        std::size_t columns);

}
