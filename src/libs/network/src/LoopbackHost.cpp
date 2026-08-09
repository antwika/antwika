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
    }

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

        if (other == nullptr)
        {
            return mine;
        }

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

        if (link == nullptr)
        {
            return;
        }

        LoopbackHost *other = link->host;

        other->forget(*this);
        forget(*other);
    }

    void LoopbackHost::send(
        PeerId peer, std::span<const std::byte> payload)
    {
        refuseOversized(payload);

        const Link *link = find(peer);

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
        return std::exchange(arrived, {});
    }

    std::vector<PeerId> LoopbackHost::peers() const
    {
        std::vector<PeerId> ids;
        ids.reserve(links.size());

        for (const Link &link : links)
        {
            ids.push_back(link.id);
        }

        return ids;

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

        std::erase_if(
            outgoing,
            [&other](const Outgoing &one) { return one.host == &other; });
    }

    void LoopbackHost::deliver(
        const Link &link, std::span<const std::byte> payload)
    {
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
        pending.push_back(
            Pending{ // GCOVR_EXCL_LINE
                .packet = std::move(packet),
                .pumpsLeft = delay});
    }

}
