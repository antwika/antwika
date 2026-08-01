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

    /** @brief How many peers one host may hold at once. */
    inline constexpr std::size_t kMaxPeers = 32;

    /** @brief The largest payload a send will carry. */
    inline constexpr std::size_t kMaxPayloadBytes = 65536;

    /** @brief The bytes a frame spends saying how long it is. */
    inline constexpr std::size_t kLengthPrefixBytes = 4;

    /**
     * @brief One listening TCP socket, and the peers reaching it.
     *
     * **Nothing here ever blocks.** Every socket is non-blocking and
     * pump() polls with a zero timeout, because this is driven from
     * inside a tick and a tick may not sleep -- see IHost::pump().
     *
     * TCP rather than UDP, because the layer above is lockstep: it
     * needs every peer's input for a tick, in order, and rebuilding
     * reliability over datagrams would be a second transport written
     * inside a backend.
     * What TCP costs in exchange is message boundaries, so a payload
     * travels behind a four-byte big-endian length -- big-endian
     * because a wire is one place a byte order has to be stated rather
     * than inherited from whichever machine wrote it.
     */
    class SocketsHost final : public IHost
    {
    public:
        /**
         * @brief Open a listening host.
         * @param logger Receives its diagnostics; must outlive it.
         * @param endpoint Where to listen; port 0 asks the operating
         * system to choose, and endpoint() then reports what it chose.
         * @throws NetworkError If the endpoint cannot be resolved or
         * the platform refused to bind or listen.
         */
        SocketsHost(ILogger &logger, const Endpoint &endpoint);

        SocketsHost(const SocketsHost &) = delete;
        SocketsHost(SocketsHost &&) = delete;

        SocketsHost &operator=(const SocketsHost &) = delete;
        SocketsHost &operator=(SocketsHost &&) = delete;

        /** @brief Close every socket this host owns. */
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
        struct Peer
        {
            PeerId id{};
            Handle handle = kNoHandle;

            /** @brief False while a connect() is still under way. */
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

} // namespace antwika::network::sockets
