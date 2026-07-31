#pragma once

namespace antwika::sound
{

    /**
     * @brief What a sound backend can actually do.
     *
     * Mirrors input::InputCapabilities, and these two flags are exactly
     * what the conformance suite skips on -- so a backend says what it
     * is rather than being asked to pretend.
     */
    struct SoundCapabilities
    {
        /** @brief Whether samples written here reach a speaker. */
        bool playback = false;

        /**
         * @brief Whether the device renders on a thread of its own.
         *
         * False means it is *pumped*: nothing happens until the caller
         * asks for frames, which is what keeps a headless run
         * instantaneous and this whole library single-threaded.
         */
        bool selfDriven = false;

        /**
         * @brief Compare two capability sets.
         * @param other The capabilities to compare against.
         * @return True when both flags match.
         */
        [[nodiscard]] bool operator==(const SoundCapabilities &other) const
            = default;
    };

} // namespace antwika::sound
