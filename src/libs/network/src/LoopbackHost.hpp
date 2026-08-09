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

    class LoopbackHost final : public IHost
    {
    public:
        LoopbackHost(LoopbackNetwork &network, Endpoint endpoint);

        LoopbackHost(const LoopbackHost &) = delete;
        LoopbackHost(LoopbackHost &&) = delete;

        LoopbackHost &operator=(const LoopbackHost &) = delete;
        LoopbackHost &operator=(LoopbackHost &&) = delete;

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
        struct Link final
        {
            PeerId id{};

            PeerId theirs{};

            LoopbackHost *host = nullptr;
        };

        struct Pending final
        {
            Packet packet;
            std::size_t pumpsLeft = 0;
        };

        struct Outgoing final
        {
            LoopbackHost *host = nullptr;
            Packet packet;
        };

        [[nodiscard]] const Link *find(PeerId peer) const;

        void forget(const LoopbackHost &other);

        void deliver(const Link &link, std::span<const std::byte> payload);

        void accept(Packet packet, std::size_t delay);

        LoopbackNetwork &net;
        Endpoint own;
        std::uint32_t nextId = 1;
        std::vector<Link> links;

        std::vector<Outgoing> outgoing;

        std::vector<Pending> pending;
        std::vector<Packet> arrived;
    };

}
