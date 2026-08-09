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

    class NullHost final : public IHost
    {
    public:
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

}
