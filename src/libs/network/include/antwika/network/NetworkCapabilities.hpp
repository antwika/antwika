#pragma once

#include <cstddef>

namespace antwika::network
{

    /**
     * @brief What a network backend can actually do.
     *
     * Mirrors sound::SoundCapabilities and input::InputCapabilities,
     * and these are exactly what the conformance suite skips on -- so a
     * backend says what it is rather than being asked to pretend.
     */
    struct NetworkCapabilities
    {
        /** @brief Whether this backend can reach another host at all. */
        bool connects = false;

        /**
         * @brief Whether a host of this backend's can be connected to.
         *
         * Separate from connects because the two are separate abilities:
         * a backend able to dial out of a network it cannot be dialled
         * into is an ordinary thing to be, and one that can do neither
         * is the null backend.
         */
        bool listens = false;

        /** @brief How many peers one host may hold at once. */
        std::size_t maxPeers = 0;

        /** @brief The largest payload a send will carry. */
        std::size_t maxPayloadBytes = 0;

        /**
         * @brief Compare two capability sets.
         * @param other The capabilities to compare against.
         * @return True when every field matches.
         */
        [[nodiscard]] bool operator==(const NetworkCapabilities &other) const
            = default;
    };

} // namespace antwika::network
