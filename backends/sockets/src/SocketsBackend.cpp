#include "SocketsBackend.hpp"

#include <memory>
#include <string>

#include <antwika/log/Level.hpp>
#include <antwika/network/NetworkError.hpp>
#include <antwika/network/Port.hpp>

#include "SocketApi.hpp"
#include "SocketsHost.hpp"

namespace antwika::network::sockets
{

    SocketsBackend::SocketsBackend(ILogger &logger) : logger(logger)
    {
        startSockets();
    }

    std::string_view SocketsBackend::name() const
    {
        return "sockets";
    }

    NetworkCapabilities SocketsBackend::capabilities() const
    {
        return NetworkCapabilities{
            .connects = true,
            .listens = true,
            .maxPeers = kMaxPeers,
            .maxPayloadBytes = kMaxPayloadBytes};
    }

    std::unique_ptr<IHost> SocketsBackend::openHost(
        const Endpoint &endpoint)
    {
        // Refused rather than handed to a resolver.
        // That would answer with this machine's own address.
        // Which is not what an empty name asked for.
        if (endpoint.host.empty())
        {
            throw NetworkError(
                "antwika::network: a host cannot be opened on port "
                + std::to_string(rawValue(endpoint.port))
                + " with no host name");
        }

        auto host = std::make_unique<SocketsHost>(logger, endpoint);

        logger.log(
            antwika::log::Level::Debug,
            "Listening on " + host->endpoint().host + ":"
                + std::to_string(rawValue(host->endpoint().port)));

        return host;
    }

} // namespace antwika::network::sockets
