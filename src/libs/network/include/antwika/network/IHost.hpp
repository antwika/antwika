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
         * @brief Get where this host can actually be reached.
         *
         * **Not necessarily the endpoint it was asked for.**
         * A backend handed port 0 is being asked to pick one, and what
         * it picked is what a peer has to dial -- so this is the
         * authority on where a host is, and the argument openHost() was
         * given is only a request.
         *
         * @return The endpoint, which does not change over its lifetime.
         */
        [[nodiscard]] virtual Endpoint endpoint() const = 0;

        /**
         * @brief Begin talking to another host.
         *
         * **Nobody being there is not an error**, and this is the one
         * place where saying so costs something worth paying for.
         * A non-blocking connect over a real network cannot know
         * whether anything is listening until several pumps later, so a
         * backend that refused an unreachable endpoint here could only
         * do it by blocking -- inside a tick, on the thread running the
         * simulation.
         * The answer arrives the same way a dropped link does instead:
         * the peer is not in peers() until it is up, and one that never
         * comes up simply never appears there.
         *
         * An in-process backend *could* answer at once, and deliberately
         * does not, since a contract two backends keep differently is
         * one a caller cannot rely on.
         *
         * @param remote Where the other host is.
         * @return This host's own name for that peer, whether or not it
         * ever becomes one.
         * @throws NetworkError If this backend cannot connect at all, or
         * it already holds as many peers as it may -- both of which are
         * a caller's mistake rather than a network's answer.
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
         * **The bytes leave on this host's next pump()**, not here.
         * A real socket takes as much as it feels like and leaves the
         * rest, so a send that wrote its payload out here would either
         * block or fail halfway; queueing it makes pump() the one place
         * bytes move, in both directions, for every backend.
         * So a caller that pumps only the far end of a link will wait
         * for something that was never sent.
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
