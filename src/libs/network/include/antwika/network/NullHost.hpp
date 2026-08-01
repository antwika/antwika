#pragma once

#include <cstddef>
#include <span>
#include <vector>

#include "antwika/network/Endpoint.hpp"
#include "antwika/network/IHost.hpp"
#include "antwika/network/Packet.hpp"
#include "antwika/network/PeerId.hpp"

namespace antwika::network
{

    /**
     * @brief A host that talks to nobody.
     *
     * **Holding no peers is an ordinary state rather than a failure**,
     * which is what lets a single-player build link the seam and open
     * one for the price of a virtual call a tick.
     * So sending, broadcasting, pumping and disconnecting all do
     * nothing at all and none of them is refused; the one thing that
     * *is* refused is connect(), because capabilities() says this
     * backend does not connect, and a backend that accepted a dial it
     * could never complete would be no use as a stand-in for one that
     * can.
     */
    class NullHost final : public IHost
    {
    public:
        /**
         * @brief Construct the host over where it says it is.
         * @param endpoint What endpoint() reports.
         */
        explicit NullHost(Endpoint endpoint);

        NullHost(const NullHost &) = delete;
        NullHost(NullHost &&) = delete;

        NullHost &operator=(const NullHost &) = delete;
        NullHost &operator=(NullHost &&) = delete;

        [[nodiscard]] Endpoint endpoint() const override;

        [[nodiscard]] PeerId connect(const Endpoint &remote) override;

        void disconnect(PeerId peer) override;

        void send(PeerId peer, std::span<const std::byte> payload) override;

        void broadcast(std::span<const std::byte> payload) override;

        void pump() override;

        [[nodiscard]] std::vector<Packet> receive() override;

        [[nodiscard]] std::vector<PeerId> peers() const override;

    private:
        Endpoint own;
    };

} // namespace antwika::network
