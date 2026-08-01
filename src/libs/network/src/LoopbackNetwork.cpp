#include "LoopbackNetwork.hpp"

#include <algorithm>
#include <string>
#include <utility>

#include "antwika/network/NetworkError.hpp"
#include "antwika/network/Port.hpp"

namespace antwika::network
{

    LoopbackNetwork::LoopbackNetwork(DeliverySchedule schedule)
        : schedule(std::move(schedule))
    {
    }

    void LoopbackNetwork::add(const Endpoint &at, LoopbackHost &host)
    {
        if (hosts.contains(at))
        {
            throw NetworkError(
                "antwika::network: a loopback host is already open at "
                + at.host + ":" + std::to_string(rawValue(at.port)));
        }

        hosts.emplace(at, &host);
    }

    void LoopbackNetwork::remove(const Endpoint &at)
    {
        hosts.erase(at);
    }

    LoopbackHost *LoopbackNetwork::find(const Endpoint &at) const
    {
        const auto found = hosts.find(at);

        if (found == hosts.end())
        {
            return nullptr;
        }

        return found->second;
    }

    bool LoopbackNetwork::carries()
    {
        const std::size_t ordinal = sends;
        ++sends;

        return std::ranges::find(schedule.dropped, ordinal)
               == schedule.dropped.end();
    }

    std::size_t LoopbackNetwork::delayPumps() const
    {
        return schedule.delayPumps;
    }

} // namespace antwika::network
