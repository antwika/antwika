#pragma once

#include <cstddef>
#include <map>

#include "antwika/network/DeliverySchedule.hpp"
#include "antwika/network/Endpoint.hpp"

namespace antwika::network
{

    class LoopbackHost;

    /**
     * @brief Where the hosts of one loopback backend find each other.
     *
     * Private, because two hosts meeting in one process is how this
     * backend is built rather than something a caller decides.
     * It holds the registry, the schedule and the one counter that
     * decides which packets are thrown away -- and no clock and no
     * generator, so a session over it is the same session every run.
     */
    class LoopbackNetwork final
    {
    public:
        /**
         * @brief Construct the network over what it does to packets.
         * @param schedule What happens to packets crossing it.
         */
        explicit LoopbackNetwork(DeliverySchedule schedule);

        LoopbackNetwork(const LoopbackNetwork &) = delete;
        LoopbackNetwork(LoopbackNetwork &&) = delete;

        LoopbackNetwork &operator=(const LoopbackNetwork &) = delete;
        LoopbackNetwork &operator=(LoopbackNetwork &&) = delete;

        /**
         * @brief Put a host on the network.
         * @param at Where it is.
         * @param host The host itself; must outlive its own removal.
         * @throws NetworkError If a host is already open there.
         */
        void add(const Endpoint &at, LoopbackHost &host);

        /**
         * @brief Take a host off the network.
         * @param at Where it was.
         */
        void remove(const Endpoint &at);

        /**
         * @brief Look a host up.
         * @param at Where to look.
         * @return The host, or null when nothing is open there.
         */
        [[nodiscard]] LoopbackHost *find(const Endpoint &at) const;

        /**
         * @brief Decide whether the next packet crosses, and count it.
         *
         * Counted across the whole network rather than per host, so a
         * schedule naming "the third packet" means the third one
         * anybody sent -- see DeliverySchedule.
         *
         * @return True when it crosses, false when it is thrown away.
         */
        [[nodiscard]] bool carries();

        /**
         * @brief Get how many pumps a packet waits before arriving.
         * @return The delay this network was scripted with.
         */
        [[nodiscard]] std::size_t delayPumps() const;

    private:
        DeliverySchedule schedule;
        std::map<Endpoint, LoopbackHost *> hosts;
        std::size_t sends = 0;
    };

} // namespace antwika::network
