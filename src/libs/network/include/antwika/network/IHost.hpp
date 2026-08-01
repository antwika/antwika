#pragma once

#include <cstddef>
#include <span>
#include <vector>

#include "antwika/network/Endpoint.hpp"
#include "antwika/network/Packet.hpp"
#include "antwika/network/PeerId.hpp"

namespace antwika::network
{

    /**
     * @brief One open transport endpoint, and the peers it is talking
     * to.
     *
     * **It owns no thread, no lock and no queue**: nothing arrives and
     * nothing leaves until pump() is called, on the thread that called
     * it, which is sound::IDevice's decision taken for sound's reasons.
     * The usual arrangement -- a reader thread with a lock-free queue --
     * would be a second concurrency model in a codebase that has none,
     * and it would make a headless two-peer test cost wall-clock time
     * where this one costs none.
     *
     * **Nothing here may reach simulation state except as an event a
     * tick asked for.** A payload is bytes until somebody upstream of a
     * recorder turns it into one, which is what keeps a run and its
     * replay agreeing; see docs and antwika::netplay for the rule
     * written out.
     */
    class IHost
    {
    public:
        virtual ~IHost() = default;

        /**
         * @brief Get where this host itself can be reached.
         * @return The endpoint it was opened at.
         */
        [[nodiscard]] virtual Endpoint endpoint() const = 0;

        /**
         * @brief Begin talking to another host.
         *
         * The peer is not in peers() until the link is up, which for a
         * backend that dials out over a real network is some pumps
         * later; a link that never comes up is one that simply never
         * appears.
         *
         * @param remote Where the other host is.
         * @return This host's own name for that peer.
         * @throws NetworkError If this backend cannot connect at all,
         * if it already holds as many peers as it may, or if the
         * endpoint is one it cannot reach.
         */
        [[nodiscard]] virtual PeerId connect(const Endpoint &remote) = 0;

        /**
         * @brief Stop talking to a peer.
         *
         * Idempotent, and safe for a peer that has already gone, so a
         * caller unwinding need not know which it holds.
         *
         * @param peer The peer to let go of.
         */
        virtual void disconnect(PeerId peer) = 0;

        /**
         * @brief Send a payload to one peer.
         *
         * **A peer this host does not hold is not an error**: a link
         * dropping between reading peers() and sending is what a
         * network does, and there is no answer a caller could give that
         * would be better than the send going nowhere.
         *
         * @param peer Who to send it to.
         * @param payload What to send; copied, so the caller's bytes
         * need not outlive the call.
         * @throws NetworkError If the payload is larger than
         * capabilities().maxPayloadBytes, which is a caller's mistake
         * rather than a network's.
         */
        virtual void send(PeerId peer, std::span<const std::byte> payload)
            = 0;

        /**
         * @brief Send a payload to every peer this host holds.
         * @param payload What to send.
         * @throws NetworkError On the same terms send() does.
         */
        virtual void broadcast(std::span<const std::byte> payload) = 0;

        /**
         * @brief Do whatever input and output is pending.
         *
         * The whole reason this library needs no thread: a host does
         * nothing until it is asked, so a headless run costs no
         * wall-clock time and a test spends none.
         */
        virtual void pump() = 0;

        /**
         * @brief Take what the last pumps collected.
         *
         * @return The packets, in the order they arrived, and none of
         * them twice: a packet is handed over once and forgotten.
         */
        [[nodiscard]] virtual std::vector<Packet> receive() = 0;

        /**
         * @brief Get every peer this host is talking to.
         * @return Their ids in ascending order, which is the order they
         * were connected in, since ids are never reused.
         */
        [[nodiscard]] virtual std::vector<PeerId> peers() const = 0;
    };

} // namespace antwika::network
