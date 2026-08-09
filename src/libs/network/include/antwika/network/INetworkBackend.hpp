#pragma once

#include <memory>
#include <string_view>

#include "antwika/network/Endpoint.hpp"
#include "antwika/network/IHost.hpp"
#include "antwika/network/NetworkCapabilities.hpp"

namespace antwika::network
{

    class INetworkBackend
    {
    public:
        virtual ~INetworkBackend() = default;

        [[nodiscard]] virtual std::string_view name() const = 0;

        [[nodiscard]] virtual NetworkCapabilities capabilities() const = 0;

        [[nodiscard]] virtual std::unique_ptr<IHost> openHost(
            const Endpoint &endpoint) = 0;
    };

}
