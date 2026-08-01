#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

#include "antwika/network/Endpoint.hpp"
#include "antwika/network/IHost.hpp"
#include "antwika/network/Packet.hpp"
#include "antwika/network/PeerId.hpp"

namespace antwika::network
{

    class LoopbackNetwork;

    /**
     * @brief One host on a loopback network.
     *
     * Private for LoopbackNetwork's reason: what a caller wants is an
     * IHost, and how two of them in one process reach each other is
     * this backend's business.
     *
     * **A link stores both names for itself** -- this host's name for
     * the peer and the peer's name for this host -- so delivering a
     * packet is never a search for who sent it.
     * The alternative was for the receiving host to look the sender up
     * by pointer, which is the same answer reached more slowly and with
     * a "not found" arm that could never be taken.
     */
    class LoopbackHost final : public IHost
    {
    public:
        /**
         * @brief Open the host on a network.
         * @param network Where its peers are; must outlive it.
         * @param endpoint Where it can be reached.
         * @throws NetworkError If a host is already open there.
         */
        LoopbackHost(LoopbackNetwork &network, Endpoint endpoint);

        LoopbackHost(const LoopbackHost &) = delete;
        LoopbackHost(LoopbackHost &&) = delete;

        LoopbackHost &operator=(const LoopbackHost &) = delete;
        LoopbackHost &operator=(LoopbackHost &&) = delete;

        /**
         * @brief Close the host, letting go of every peer.
         *
         * Every peer is told, so no host is left holding a link to
         * something that has gone.
         */
        ~LoopbackHost() override;

        [[nodiscard]] Endpoint endpoint() const override;

        [[nodiscard]] PeerId connect(const Endpoint &remote) override;

        void disconnect(PeerId peer) override;

        void send(PeerId peer, std::span<const std::byte> payload) override;

        void broadcast(std::span<const std::byte> payload) override;

        void pump() override;

        [[nodiscard]] std::vector<Packet> receive() override;

        [[nodiscard]] std::vector<PeerId> peers() const override;

    private:
        struct Link
        {
            /** @brief What this host calls the peer. */
            PeerId id{};

            /** @brief What the peer calls this host. */
            PeerId theirs{};

            /** @brief The peer itself. */
            LoopbackHost *host = nullptr;
        };

        struct Pending
        {
            Packet packet;
            std::size_t pumpsLeft = 0;
        };

        [[nodiscard]] const Link *find(PeerId peer) const;

        void forget(const LoopbackHost &other);

        void deliver(const Link &link, std::span<const std::byte> payload);

        void accept(Packet packet, std::size_t delay);

        LoopbackNetwork &net;
        Endpoint own;
        std::uint32_t nextId = 1;
        std::vector<Link> links;
        std::vector<Pending> pending;
        std::vector<Packet> arrived;
    };

} // namespace antwika::network
