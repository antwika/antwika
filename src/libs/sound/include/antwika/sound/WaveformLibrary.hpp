#pragma once

#include <cstddef>
#include <vector>

#include "antwika/sound/Waveform.hpp"
#include "antwika/sound/WaveformId.hpp"

namespace antwika::sound
{

    /**
     * @brief Owns every waveform something might play.
     *
     * It exists so that nothing on the render path holds a pointer that
     * could dangle: a voice refers to a waveform the library owns, and
     * the library outlives the mixer that reads it.
     *
     * Adding is a simulation-side operation and throws; resolving an id
     * to a waveform happens in play(), never inside render().
     */
    class WaveformLibrary final
    {
    public:
        /**
         * @brief Take ownership of a waveform.
         * @param waveform The audio to hold.
         * @return The id it can be played by.
         * @throws SoundError If the waveform does not hold exactly the
         * samples its format claims, or holds no frames at all.
         */
        WaveformId add(Waveform waveform);

        /**
         * @brief Get a waveform back.
         * @param id The id to resolve.
         * @return The waveform.
         * @throws SoundError If no waveform has that id.
         */
        [[nodiscard]] const Waveform &get(WaveformId id) const;

        /**
         * @brief Get how many waveforms are held.
         * @return The count.
         */
        [[nodiscard]] std::size_t size() const noexcept;

    private:
        std::vector<Waveform> held;
    };

} // namespace antwika::sound
