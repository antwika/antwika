#pragma once

#include <compare>
#include <string>

#include "antwika/network/Port.hpp"

namespace antwika::network
{

    /**
     * @brief Where a host can be reached.
     *
     * A name and a port rather than a resolved address, because
     * resolving one is a backend's business: the null backend resolves
     * nothing at all, and the loopback one matches on the name it was
     * given without ever asking an operating system.
     */
    struct Endpoint
    {
        /** @brief The host, as a name or as an address literal. */
        std::string host;

        /** @brief The port on it. */
        Port port{};

        /**
         * @brief Compare two endpoints.
         * @param other The endpoint to compare against.
         * @return True when the host and the port both match.
         */
        [[nodiscard]] bool operator==(const Endpoint &other) const
            = default;

        /**
         * @brief Order two endpoints, by host and then by port.
         *
         * Ordered so that a host can be looked up in an ordered map,
         * which is what LoopbackBackend keeps its hosts in; nothing
         * about a network depends on which of two endpoints sorts
         * first.
         *
         * @param other The endpoint to compare against.
         * @return How this one orders against it.
         */
        [[nodiscard]] std::strong_ordering operator<=>(
            const Endpoint &other) const = default;
    };

} // namespace antwika::network
