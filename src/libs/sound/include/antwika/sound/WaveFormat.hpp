#pragma once

#include <cstdint>

namespace antwika::sound
{

    /**
     * @brief Frames of audio per second.
     *
     * A plain alias rather than a scoped enum, following time::Tick: the
     * strongly-typed-id idiom is for handles, and a rate is a quantity
     * that gets multiplied.
     */
    using SampleRate = std::uint32_t;

    /** @brief What a device is opened at unless something says otherwise. */
    inline constexpr SampleRate kDefaultSampleRate = 48000;

    /** @brief How many channels one frame holds. */
    using ChannelCount = std::uint8_t;

    inline constexpr ChannelCount kMono = 1;
    inline constexpr ChannelCount kStereo = 2;

    /**
     * @brief The most channels anything here will carry.
     *
     * A ceiling rather than a limitation: it is what makes a malformed
     * file's channel count refusable instead of an allocation.
     */
    inline constexpr ChannelCount kMaxChannels = 8;

    /**
     * @brief How a stream of audio is laid out in time.
     *
     * **There is deliberately no sample-format member.** A decoded
     * Waveform is always normalised float whatever the file held, in
     * exactly the way a gfx::Bitmap is always straight 8-bit RGBA
     * whatever the PNG held -- the storage format is the decoder's
     * private business, and admitting it here would be a conversion
     * matrix every caller pays for.
     */
    struct WaveFormat
    {
        SampleRate rate = kDefaultSampleRate;
        ChannelCount channels = kStereo;

        /**
         * @brief Check that this format describes audio at all.
         * @return True when the rate is non-zero and the channel count is
         * between one and kMaxChannels.
         */
        [[nodiscard]] bool isValid() const noexcept;

        /**
         * @brief Compare two formats.
         * @param other The format to compare against.
         * @return True when the rate and the channel count both match.
         */
        [[nodiscard]] bool operator==(const WaveFormat &other) const
            = default;
    };

} // namespace antwika::sound
