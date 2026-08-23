#pragma once

#include <cstddef>
#include <vector>

#include "antwika/sound/Waveform.hpp"
#include "antwika/sound/WaveformId.hpp"

namespace antwika::sound
{

    class WaveformLibrary final
    {
    public:
        WaveformId add(Waveform waveform);

        [[nodiscard]] const Waveform &getWaveform(WaveformId idWaveform) const;

        [[nodiscard]] std::size_t getSize() const noexcept;

    private:
        std::vector<Waveform> waveforms;
    };

}
