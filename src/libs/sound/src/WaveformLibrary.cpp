#include "antwika/sound/WaveformLibrary.hpp"

#include <cstddef>
#include <string>
#include <utility>

#include "antwika/sound/SoundError.hpp"

namespace antwika::sound
{

    WaveformId WaveformLibrary::add(Waveform waveform)
    {
        if (!waveform.isValid())
        {
            throw SoundError(
                "antwika::sound: a waveform does not hold a whole number "
                "of frames of the format it names");
        }

        if (waveform.frameCount() == 0)
        {
            throw SoundError(
                "antwika::sound: a waveform holding no frames could never "
                "be played");
        }

        waveforms.push_back(std::move(waveform));

        return static_cast<WaveformId>(waveforms.size() - 1);
    }

    const Waveform &WaveformLibrary::get(WaveformId idWaveform) const
    {
        const auto index = static_cast<std::size_t>(rawValue(idWaveform));

        if (index >= waveforms.size())
        {
            throw SoundError(
                "antwika::sound: no waveform has id "
                + std::to_string(rawValue(idWaveform)));
        }

        return waveforms[index];
    }

    std::size_t WaveformLibrary::size() const noexcept
    {
        return waveforms.size();
    }

}
