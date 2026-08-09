#pragma once

#include <cstddef>
#include <map>

#include "antwika/network/DeliverySchedule.hpp"
#include "antwika/network/Endpoint.hpp"

namespace antwika::network
{

    class LoopbackHost;

    class LoopbackNetwork final
    {
    public:
        explicit LoopbackNetwork(DeliverySchedule schedule);

        LoopbackNetwork(const LoopbackNetwork &) = delete;
        LoopbackNetwork(LoopbackNetwork &&) = delete;

        LoopbackNetwork &operator=(const LoopbackNetwork &) = delete;
        LoopbackNetwork &operator=(LoopbackNetwork &&) = delete;

        void add(const Endpoint &at, LoopbackHost &host);

        void remove(const Endpoint &at);

        [[nodiscard]] LoopbackHost *find(const Endpoint &at) const;

        [[nodiscard]] bool carries();

        [[nodiscard]] std::size_t delayPumps() const;

    private:
        DeliverySchedule schedule;
        std::map<Endpoint, LoopbackHost *> hosts;
        std::size_t sends = 0;
    };

}
