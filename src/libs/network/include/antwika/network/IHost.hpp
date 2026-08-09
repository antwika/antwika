#pragma once

#include <cstddef>
#include <span>
#include <vector>

#include "antwika/network/Endpoint.hpp"
#include "antwika/network/Packet.hpp"
#include "antwika/network/PeerId.hpp"

namespace antwika::network
{

    class IHost
    {
    public:
        virtual ~IHost() = default;

        [[nodiscard]] virtual Endpoint endpoint() const = 0;

        [[nodiscard]] virtual PeerId connect(const Endpoint &remote) = 0;

        virtual void disconnect(PeerId peer) = 0;

        virtual void send(PeerId peer, std::span<const std::byte> payload)
            = 0;

        virtual void broadcast(std::span<const std::byte> payload) = 0;

        virtual void pump() = 0;

        [[nodiscard]] virtual std::vector<Packet> receive() = 0;

        [[nodiscard]] virtual std::vector<PeerId> peers() const = 0;
    };

}
