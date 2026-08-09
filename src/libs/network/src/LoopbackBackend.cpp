#include "antwika/network/LoopbackBackend.hpp"

#include <memory>
#include <string>
#include <utility>

#include <antwika/log/Level.hpp>

#include "antwika/network/NetworkError.hpp"
#include "antwika/network/Port.hpp"

#include "LoopbackHost.hpp"
#include "LoopbackNetwork.hpp"

namespace antwika::network
{

    LoopbackBackend::LoopbackBackend(
        ILogger &logger, DeliverySchedule schedule)
        : logger(logger),
          network(std::make_unique<LoopbackNetwork>(std::move(schedule)))
    {
    }

    LoopbackBackend::~LoopbackBackend() = default;

    std::string_view LoopbackBackend::name() const
    {
        return "loopback";
    }

    NetworkCapabilities LoopbackBackend::capabilities() const
    {
        return NetworkCapabilities{
            .connects = true,
            .listens = true,
            .maxPeers = kLoopbackMaxPeers,
            .maxPayloadBytes = kLoopbackMaxPayloadBytes};
    }

    std::unique_ptr<IHost> LoopbackBackend::openHost(
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
            antwika::log::Level::Debug,
            "Opening a loopback network host");

        return std::make_unique<LoopbackHost>(*network, endpoint);
    }

}
