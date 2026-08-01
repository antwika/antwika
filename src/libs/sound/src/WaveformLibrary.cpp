#include "antwika/sound/WaveformLibrary.hpp"

#include <cstddef>
#include <string>
#include <utility>

#include "antwika/sound/SoundError.hpp"

namespace antwika::sound
{

    WaveformId WaveformLibrary::add(Waveform waveform)
    {
        if (!waveform.isComplete())
        {
            throw SoundError(
                "antwika::sound: a waveform does not hold a whole number "
                "of frames of the format it names");
        }

        // A voice that can never produce a sample is not a thing to play.
        // Refused here rather than guarded at every read of it.
        // A looping voice would otherwise restart on a missing frame.
        // It would then read past the end of the samples.
        if (waveform.frameCount() == 0)
        {
            throw SoundError(
                "antwika::sound: a waveform holding no frames could never "
                "be played");
        }

        held.push_back(std::move(waveform));

        return static_cast<WaveformId>(held.size() - 1);
    }

    const Waveform &WaveformLibrary::get(WaveformId id) const
    {
        const auto index = static_cast<std::size_t>(rawValue(id));

        if (index >= held.size())
        {
            throw SoundError(
                "antwika::sound: no waveform has id "
                + std::to_string(rawValue(id)));
        }

        return held[index];
    }

    std::size_t WaveformLibrary::size() const noexcept
    {
        return held.size();
    }

} // namespace antwika::sound
