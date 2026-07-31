#pragma once

#include <cstddef>
#include <vector>

#include "antwika/sound/Frames.hpp"
#include "antwika/sound/WaveFormat.hpp"

namespace antwika::sound
{

    /**
     * @brief Decoded audio ready to be played, as a plain value.
     *
     * The gfx::Bitmap of this library: decoded once, handed to whatever
     * plays it, and not aliased by anything afterwards -- so nothing here
     * owns a resource and nothing here has a lifetime rule.
     *
     * Samples are **interleaved** and normalised to roughly -1 to +1.
     * Interleaved because that is what every file holds and what one
     * EXPECT_EQ compares; the buffer a device renders into is planar
     * instead, and the mixer is what crosses between the two.
     */
    struct Waveform
    {
        WaveFormat format;

        /** @brief Frame-major, channels interleaved within each frame. */
        std::vector<float> samples;

        /**
         * @brief Get how many frames this holds.
         * @return The sample count divided by the channel count, or zero
         * when the format names no channels.
         */
        [[nodiscard]] FrameCount frameCount() const noexcept;

        /**
         * @brief Check that this holds exactly the samples it claims to.
         *
         * Every voice checks this before reading, so it lives here
         * rather than once per caller -- the same reason
         * gfx::Bitmap::isComplete() does.
         *
         * @return True when the format is valid and the sample count is
         * a whole number of frames.
         */
        [[nodiscard]] bool isComplete() const noexcept;

        /**
         * @brief Compare two waveforms.
         * @param other The waveform to compare against.
         * @return True when the formats match and every sample does.
         */
        [[nodiscard]] bool operator==(const Waveform &other) const = default;
    };

} // namespace antwika::sound
