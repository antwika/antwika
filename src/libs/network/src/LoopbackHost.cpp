#include "LoopbackHost.hpp"

#include <algorithm>
#include <string>
#include <utility>

#include "antwika/network/LoopbackBackend.hpp"
#include "antwika/network/NetworkError.hpp"
#include "antwika/network/Port.hpp"

#include "LoopbackNetwork.hpp"

namespace antwika::network
{

    namespace
    {
        std::string nameOf(const Endpoint &endpoint)
        {
            return endpoint.host + ":"
                   + std::to_string(rawValue(endpoint.port));
        }

        void refuseOversized(std::span<const std::byte> payload)
        {
            if (payload.size() > kLoopbackMaxPayloadBytes)
            {
                throw NetworkError(
                    "antwika::network: a payload of "
                    + std::to_string(payload.size())
                    + " bytes is larger than a loopback send carries");
            }
        }
    } // namespace

    LoopbackHost::LoopbackHost(
        LoopbackNetwork &network, Endpoint endpoint)
        : net(network), own(std::move(endpoint))
    {
        net.add(own, *this);
    }

    LoopbackHost::~LoopbackHost()
    {
        for (const Link &link : links)
        {
            link.host->forget(*this);
        }

        net.remove(own);
    }

    Endpoint LoopbackHost::endpoint() const
    {
        return own;
    }

    PeerId LoopbackHost::connect(const Endpoint &remote)
    {
        LoopbackHost *other = net.find(remote);

        // Refused rather than looped back to itself.
        // Its own peer would be two links to one place.
        if (other == this)
        {
            throw NetworkError(
                "antwika::network: a host cannot connect to itself at "
                + nameOf(remote));
        }

        if (links.size() >= kLoopbackMaxPeers
            || (other != nullptr
                && other->links.size() >= kLoopbackMaxPeers))
        {
            throw NetworkError(
                "antwika::network: a loopback host holds at most "
                + std::to_string(kLoopbackMaxPeers) + " peers");
        }

        const PeerId mine{nextId};
        ++nextId;

        // Nobody there is not an error -- see IHost::connect().
        // This backend could answer at once and deliberately does not.
        // A name is handed back that simply never becomes a peer.
        if (other == nullptr)
        {
            return mine;
        }

        // Both names are settled here.
        // So neither end ever searches for what the other calls it.
        const PeerId theirs{other->nextId};
        ++other->nextId;

        links.push_back(Link{.id = mine, .theirs = theirs, .host = other});
        other->links.push_back(
            Link{.id = theirs, .theirs = mine, .host = this});

        return mine;
    }

    void LoopbackHost::disconnect(PeerId peer)
    {
        const Link *link = find(peer);

        // Idempotent, and safe for a peer that has already gone.
        if (link == nullptr)
        {
            return;
        }

        // Both ends forget each other.
        // That takes the link and whatever was queued for it.
        // Erasing this side's link alone left a queued send to make.
        LoopbackHost *other = link->host;

        other->forget(*this);
        forget(*other);
    }

    void LoopbackHost::send(
        PeerId peer, std::span<const std::byte> payload)
    {
        refuseOversized(payload);

        const Link *link = find(peer);

        // A peer this host does not hold is not an error -- see IHost.
        if (link == nullptr)
        {
            return;
        }

        deliver(*link, payload);
    }

    void LoopbackHost::broadcast(std::span<const std::byte> payload)
    {
        refuseOversized(payload);

        for (const Link &link : links)
        {
            deliver(link, payload);
        }
    }

    void LoopbackHost::pump()
    {
        for (Outgoing &one : outgoing)
        {
            one.host->accept(std::move(one.packet), net.delayPumps());
        }

        outgoing.clear();

        std::vector<Pending> waiting;

        for (Pending &held : pending)
        {
            if (held.pumpsLeft == 0)
            {
                arrived.push_back(std::move(held.packet));
                continue;
            }

            --held.pumpsLeft;
            waiting.push_back(std::move(held));
        }

        pending = std::move(waiting);
    }

    std::vector<Packet> LoopbackHost::receive()
    {
        // Handed over once and forgotten, so no packet arrives twice.
        return std::exchange(arrived, {});
    }

    std::vector<PeerId> LoopbackHost::peers() const
    {
        std::vector<PeerId> ids;
        ids.reserve(links.size());

        // Ascending without being sorted.
        // Every link held was appended with this host's own counter.
        // And letting one go keeps the order of the rest.
        for (const Link &link : links)
        {
            ids.push_back(link.id);
        }

        return ids;

        // The cleanup destructing `ids` should this function unwind.
        // Nothing between the reserve and the return ever does.
        // Signature (b) in docs/confirming-unreachable-branches.md.
    } // GCOVR_EXCL_LINE

    const LoopbackHost::Link *LoopbackHost::find(PeerId peer) const
    {
        for (const Link &link : links)
        {
            if (link.id == peer)
            {
                return &link;
            }
        }

        return nullptr;
    }

    void LoopbackHost::forget(const LoopbackHost &other)
    {
        std::erase_if(
            links,
            [&other](const Link &held) { return held.host == &other; });

        // Anything still queued for that host goes with the link.
        // So pump() never hands a packet to something that has gone.
        std::erase_if(
            outgoing,
            [&other](const Outgoing &one) { return one.host == &other; });
    }

    void LoopbackHost::deliver(
        const Link &link, std::span<const std::byte> payload)
    {
        // Counted whether or not it crosses.
        // So an ordinal names the same packet however it is scripted.
        if (!net.carries())
        {
            return;
        }

        outgoing.push_back(
            Outgoing{
                .host = link.host,
                .packet = Packet{
                    .from = link.theirs,
                    .payload = {payload.begin(), payload.end()}}});
    }

    void LoopbackHost::accept(Packet packet, std::size_t delay)
    {
        // The unwind edge is push_back's own allocation failing.
        // And the cleanup of the Pending that would follow it.
        // Signature (a) in docs/confirming-unreachable-branches.md.
        pending.push_back(
            Pending{ // GCOVR_EXCL_LINE
                .packet = std::move(packet),
                .pumpsLeft = delay});
    }

} // namespace antwika::network
