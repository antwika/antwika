#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

#include <antwika/log/ILogger.hpp>
#include <antwika/network/Endpoint.hpp>
#include <antwika/network/IHost.hpp>
#include <antwika/network/Packet.hpp>
#include <antwika/network/PeerId.hpp>

#include "SocketApi.hpp"

namespace antwika::network::sockets
{

    using antwika::log::ILogger;

    inline constexpr std::size_t kMaxPeers = 32;

    inline constexpr std::size_t kMaxPayloadBytes = 65536;

    inline constexpr std::size_t kLengthPrefixBytes = 4;

    class SocketsHost final : public IHost
    {
    public:
        SocketsHost(ILogger &logger, const Endpoint &endpoint);

        SocketsHost(const SocketsHost &) = delete;
        SocketsHost(SocketsHost &&) = delete;

        SocketsHost &operator=(const SocketsHost &) = delete;
        SocketsHost &operator=(SocketsHost &&) = delete;

        ~SocketsHost() override;

        [[nodiscard]] Endpoint endpoint() const override;

        [[nodiscard]] PeerId connect(const Endpoint &remote) override;

        void disconnect(PeerId peer) override;

        void send(PeerId peer, std::span<const std::byte> payload) override;

        void broadcast(std::span<const std::byte> payload) override;

        void pump() override;

        [[nodiscard]] std::vector<Packet> receive() override;

        [[nodiscard]] std::vector<PeerId> peers() const override;

    private:
        struct Peer final
        {
            PeerId id{};
            Handle handle = kNoHandle;

            bool up = false;

            std::vector<std::byte> outbound;
            std::vector<std::byte> inbound;
        };

        [[nodiscard]] Peer *find(PeerId peer);

        void queue(Peer &peer, std::span<const std::byte> payload);

        void acceptWaiting();
        void finishConnect(Peer &peer);
        void readFrom(Peer &peer);
        void writeTo(Peer &peer);
        void takeFrames(Peer &peer);
        void drop(Peer &peer);

        ILogger &logger;
        Handle listener = kNoHandle;
        Endpoint own;
        std::uint32_t nextId = 1;
        std::vector<Peer> links;
        std::vector<Packet> arrived;
    };

}
