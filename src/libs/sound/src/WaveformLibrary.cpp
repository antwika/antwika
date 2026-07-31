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
