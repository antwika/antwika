#pragma once

#include <cstddef>
#include <vector>

#include "antwika/network/PeerId.hpp"

namespace antwika::network
{

    /**
     * @brief One payload, and which peer it came from.
     *
     * Bytes rather than anything shaped, because this library carries
     * what it is handed and never reads it: what a payload means is
     * decided by whatever lays a session over this, which is what keeps
     * a transport testable without one.
     */
    struct Packet
    {
        /** @brief The peer that sent it, as this host names one. */
        PeerId from{};

        /** @brief What was sent, exactly as it was sent. */
        std::vector<std::byte> payload;

        /**
         * @brief Compare two packets.
         * @param other The packet to compare against.
         * @return True when the sender and every byte match.
         */
        [[nodiscard]] bool operator==(const Packet &other) const = default;
    };

} // namespace antwika::network
