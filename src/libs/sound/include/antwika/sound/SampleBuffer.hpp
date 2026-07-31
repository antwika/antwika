#pragma once

#include <span>

#include "antwika/sound/Frames.hpp"
#include "antwika/sound/WaveFormat.hpp"

namespace antwika::sound
{

    /**
     * @brief Where a device wants its next frames written.
     *
     * **Non-owning and planar**, which is two decisions.
     * Non-owning because nothing on the render path may allocate.
     * Planar -- one span per channel rather than one interleaved span --
     * because that is what mixing wants, where storage wants interleaved.
     *
     * A struct of spans rather than a class with a validating
     * constructor, because render() is noexcept and nothing on that path
     * may throw; the *device* asserts isComplete() before it calls.
     */
    struct SampleBuffer
    {
        /** @brief One span of `frames` samples per channel. */
        std::span<const std::span<float>> channels;

        /** @brief How many frames each channel's span must hold. */
        FrameCount frames = 0;

        /**
         * @brief Get how many channels this buffer carries.
         * @return The channel count.
         */
        [[nodiscard]] ChannelCount channelCount() const noexcept;

        /**
         * @brief Check that every channel really holds `frames` samples.
         * @return True when the channel count is usable and no span is
         * shorter than `frames`.
         */
        [[nodiscard]] bool isComplete() const noexcept;

        /**
         * @brief Write silence over every channel.
         *
         * What a callback does before mixing into it, and what one that
         * has nothing to play does instead of leaving whatever the
         * device handed it.
         */
        void silence() const noexcept;
    };

} // namespace antwika::sound
