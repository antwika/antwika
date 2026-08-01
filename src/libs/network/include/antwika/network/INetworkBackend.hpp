#pragma once

#include <memory>
#include <string_view>

#include "antwika/network/Endpoint.hpp"
#include "antwika/network/IHost.hpp"
#include "antwika/network/NetworkCapabilities.hpp"

namespace antwika::network
{

    /**
     * @brief Opens hosts, and reports what it can do.
     *
     * The one seam between Antwika and a concrete transport.
     * Exactly one implementation is compiled into a given build, chosen
     * by ANTWIKA_NETWORK_BACKEND, so no code above this interface names
     * a socket API.
     */
    class INetworkBackend
    {
    public:
        virtual ~INetworkBackend() = default;

        /**
         * @brief Get what this backend is called.
         * @return Its name, which does not change over its lifetime.
         */
        [[nodiscard]] virtual std::string_view name() const = 0;

        /**
         * @brief Get what this backend can do.
         * @return Its capabilities, which do not change either.
         */
        [[nodiscard]] virtual NetworkCapabilities capabilities() const = 0;

        /**
         * @brief Open a host.
         * @param endpoint Where to open it; a backend that cannot be
         * connected to still takes one, since it is what the host
         * reports about itself.
         * @return The host, never null.
         * @throws NetworkError If the endpoint is not one this backend
         * can open, or one of its hosts is already open there.
         */
        [[nodiscard]] virtual std::unique_ptr<IHost> openHost(
            const Endpoint &endpoint) = 0;
    };

} // namespace antwika::network
