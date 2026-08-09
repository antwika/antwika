#include "antwika/network/NullNetworkBackend.hpp"

#include <memory>
#include <string>

#include <antwika/log/Level.hpp>

#include "antwika/network/NetworkError.hpp"
#include "antwika/network/NullHost.hpp"

namespace antwika::network
{

    NullNetworkBackend::NullNetworkBackend(ILogger &logger)
        : logger(logger)
    {
    }

    std::string_view NullNetworkBackend::name() const
    {
        return "null";
    }

    NetworkCapabilities NullNetworkBackend::capabilities() const
    {
        return NetworkCapabilities{
            .connects = false,
            .listens = false,
            .maxPeers = 0,
            .maxPayloadBytes = 0};
    }

    std::unique_ptr<IHost> NullNetworkBackend::openHost(
        const Endpoint &endpoint)
    {
        if (endpoint.host.empty())
        {
            throw NetworkError(
                "antwika::network: a host cannot be opened on port "
                + std::to_string(rawValue(endpoint.port))
                + " with no host name");
        }

        logger.log(
            antwika::log::Level::Debug, "Opening a null network host");

        return std::make_unique<NullHost>(endpoint);
    }

}
