#include "SocketsHost.hpp"

#include <algorithm>
#include <array>
#include <cstring>
#include <string>
#include <utility>

#include <antwika/log/Level.hpp>
#include <antwika/network/NetworkError.hpp>
#include <antwika/network/Port.hpp>

#ifdef _WIN32
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#endif

namespace antwika::network::sockets
{

    namespace
    {
        constexpr std::size_t kReadChunk = 4096;

#ifdef _WIN32
        constexpr int kSendFlags = 0;
#else
        constexpr int kSendFlags = MSG_NOSIGNAL;
#endif

        std::string nameOf(const Endpoint &endpoint)
        {
            return endpoint.host + ":"
                   + std::to_string(rawValue(endpoint.port));
        }

        ::sockaddr_in resolve(const Endpoint &endpoint)
        {
            ::addrinfo hints{};
            hints.ai_family = AF_INET;
            hints.ai_socktype = SOCK_STREAM;

            ::addrinfo *found = nullptr;
            const int outcome = ::getaddrinfo(
                endpoint.host.c_str(), nullptr, &hints, &found);

            if (outcome != 0 || found == nullptr)
            {
                throw NetworkError(
                    "antwika::network: cannot resolve " + endpoint.host);
            }

            ::sockaddr_in address{};
            std::memcpy(&address, found->ai_addr, sizeof(address));
            ::freeaddrinfo(found);

            address.sin_port = ::htons(rawValue(endpoint.port));

            return address;
        }

        Endpoint describe(const ::sockaddr_in &address)
        {
            std::array<char, INET_ADDRSTRLEN> text{};
            ::inet_ntop(
                AF_INET, &address.sin_addr, text.data(), text.size());

            return Endpoint{
                .host = std::string(text.data()),
                .port = Port{::ntohs(address.sin_port)}};
        }

        void appendLength(std::vector<std::byte> &out, std::size_t size)
        {
            for (std::size_t shift = kLengthPrefixBytes; shift > 0;
                 --shift)
            {
                const auto byte =
                    (size >> ((shift - 1) * 8)) & 0xFFU;

                out.push_back(static_cast<std::byte>(byte));
            }
        }

        std::size_t readLength(const std::vector<std::byte> &in)
        {
            std::size_t size = 0;

            for (std::size_t index = 0; index < kLengthPrefixBytes;
                 ++index)
            {
                size = (size << 8) | std::to_integer<std::size_t>(
                           in[index]);
            }

            return size;
        }
    }

    SocketsHost::SocketsHost(ILogger &logger, const Endpoint &endpoint)
        : logger(logger)
    {
        const ::sockaddr_in wanted = resolve(endpoint);

        listener = openTcp();

        if (listener == kNoHandle)
        {
            throw NetworkError(
                "antwika::network: no socket for " + nameOf(endpoint)
                + ": " + lastErrorText());
        }

        allowReuse(listener);

        if (!makeNonBlocking(listener)
            || ::bind(
                   listener,
                   reinterpret_cast<const ::sockaddr *>(&wanted),
                   sizeof(wanted))
                   != 0
            || ::listen(listener, static_cast<int>(kMaxPeers)) != 0)
        {
            const std::string why = lastErrorText();

            closeSocket(listener);
            listener = kNoHandle;

            throw NetworkError(
                "antwika::network: cannot listen at " + nameOf(endpoint)
                + ": " + why);
        }

        ::sockaddr_in bound{};
#ifdef _WIN32
        int size = static_cast<int>(sizeof(bound));
#else
        ::socklen_t size = sizeof(bound);
#endif
        ::getsockname(
            listener, reinterpret_cast<::sockaddr *>(&bound), &size);

        own = describe(bound);
    }

    SocketsHost::~SocketsHost()
    {
        for (const Peer &peer : links)
        {
            closeSocket(peer.handle);
        }

        closeSocket(listener);
    }

    Endpoint SocketsHost::endpoint() const
    {
        return own;
    }

    PeerId SocketsHost::connect(const Endpoint &remote)
    {
        if (links.size() >= kMaxPeers)
        {
            throw NetworkError(
                "antwika::network: a host holds at most "
                + std::to_string(kMaxPeers) + " peers");
        }

        const ::sockaddr_in target = resolve(remote);
        const Handle handle = openTcp();

        if (handle == kNoHandle || !makeNonBlocking(handle))
        {
            closeSocket(handle);

            throw NetworkError(
                "antwika::network: no socket for " + nameOf(remote)
                + ": " + lastErrorText());
        }

        const PeerId mine{nextId};
        ++nextId;

        Peer peer{
            .id = mine,
            .handle = handle,
            .up = false,
            .outbound = {},
            .inbound = {}};

        if (::connect(
                handle,
                reinterpret_cast<const ::sockaddr *>(&target),
                sizeof(target))
            == 0)
        {
            peer.up = true;
        }
        else if (!lastIsInProgress())
        {
            closeSocket(handle);

            return mine;
        }

        links.push_back(std::move(peer));

        return mine;
    }

    void SocketsHost::disconnect(PeerId peer)
    {
        Peer *held = find(peer);

        if (held == nullptr)
        {
            return;
        }

        closeSocket(held->handle);

        std::erase_if(
            links, [peer](const Peer &one) { return one.id == peer; });
    }

    void SocketsHost::send(
        PeerId peer, std::span<const std::byte> payload)
    {
        if (payload.size() > kMaxPayloadBytes)
        {
            throw NetworkError(
                "antwika::network: a payload of "
                + std::to_string(payload.size())
                + " bytes is larger than a send carries");
        }

        Peer *held = find(peer);

        if (held == nullptr)
        {
            return;
        }

        queue(*held, payload);
    }

    void SocketsHost::broadcast(std::span<const std::byte> payload)
    {
        if (payload.size() > kMaxPayloadBytes)
        {
            throw NetworkError(
                "antwika::network: a payload of "
                + std::to_string(payload.size())
                + " bytes is larger than a send carries");
        }

        for (Peer &peer : links)
        {
            queue(peer, payload);
        }
    }

    void SocketsHost::queue(
        Peer &peer, std::span<const std::byte> payload)
    {
        appendLength(peer.outbound, payload.size());
        peer.outbound.insert(
            peer.outbound.end(), payload.begin(), payload.end());
    }

    void SocketsHost::pump()
    {
        acceptWaiting();

        std::vector<PollFd> asking;
        asking.reserve(links.size());

        for (const Peer &peer : links)
        {
            PollFd one{};
            one.fd = peer.handle;
            one.events = static_cast<short>(
                peer.up ? POLLIN : POLLOUT);

            if (peer.up && !peer.outbound.empty())
            {
                one.events =
                    static_cast<short>(one.events | POLLOUT);
            }

            asking.push_back(one);
        }

        if (pollNow(asking.data(), asking.size()) < 0)
        {
            return;
        }

        for (std::size_t index = 0; index < links.size(); ++index)
        {
            const short ready = asking[index].revents;

            if (!links[index].up)
            {
                if ((ready & (POLLOUT | POLLERR | POLLHUP)) != 0)
                {
                    finishConnect(links[index]);
                }

                continue;
            }

            if ((ready & POLLOUT) != 0)
            {
                writeTo(links[index]);
            }

            if ((ready & (POLLIN | POLLERR | POLLHUP)) != 0)
            {
                readFrom(links[index]);
            }
        }

        std::erase_if(
            links,
            [](const Peer &peer) { return peer.handle == kNoHandle; });
    }

    void SocketsHost::acceptWaiting()
    {
        while (links.size() < kMaxPeers)
        {
            const Handle taken = ::accept(listener, nullptr, nullptr);

            if (taken == kNoHandle)
            {
                return;
            }

            if (!makeNonBlocking(taken))
            {
                closeSocket(taken);

                continue;
            }

            links.push_back(
                Peer{
                    .id = PeerId{nextId},
                    .handle = taken,
                    .up = true,
                    .outbound = {},
                    .inbound = {}});

            ++nextId;
        }
    }

    void SocketsHost::finishConnect(Peer &peer)
    {
        if (pendingError(peer.handle) != 0)
        {
            drop(peer);

            return;
        }

        peer.up = true;
    }

    void SocketsHost::writeTo(Peer &peer)
    {
        while (!peer.outbound.empty())
        {
            const auto written = ::send(
                peer.handle,
                reinterpret_cast<const char *>(peer.outbound.data()),
                static_cast<int>(peer.outbound.size()),
                kSendFlags);

            if (written <= 0)
            {
                if (!lastWouldBlock())
                {
                    drop(peer);
                }

                return;
            }

            peer.outbound.erase(
                peer.outbound.begin(),
                peer.outbound.begin() + written);
        }
    }

    void SocketsHost::readFrom(Peer &peer)
    {
        std::array<char, kReadChunk> chunk{};

        while (true)
        {
            const auto read = ::recv(
                peer.handle,
                chunk.data(),
                static_cast<int>(chunk.size()),
                0);

            if (read == 0)
            {
                takeFrames(peer);
                drop(peer);

                return;
            }

            if (read < 0)
            {
                if (!lastWouldBlock())
                {
                    takeFrames(peer);
                    drop(peer);

                    return;
                }

                takeFrames(peer);

                return;
            }

            const auto *bytes =
                reinterpret_cast<const std::byte *>(chunk.data());

            peer.inbound.insert(
                peer.inbound.end(), bytes, bytes + read);
        }
    }

    void SocketsHost::takeFrames(Peer &peer)
    {
        while (peer.inbound.size() >= kLengthPrefixBytes)
        {
            const std::size_t size = readLength(peer.inbound);

            if (size > kMaxPayloadBytes)
            {
                logger.log(
                    antwika::log::Level::Warning,
                    "Dropping a peer that framed "
                        + std::to_string(size) + " bytes");

                drop(peer);

                return;
            }

            if (peer.inbound.size() < kLengthPrefixBytes + size)
            {
                return;
            }

            const auto start =
                peer.inbound.begin()
                + static_cast<std::ptrdiff_t>(kLengthPrefixBytes);

            arrived.push_back(
                Packet{
                    .from = peer.id,
                    .payload = {
                        start,
                        start + static_cast<std::ptrdiff_t>(size)}});

            peer.inbound.erase(
                peer.inbound.begin(),
                start + static_cast<std::ptrdiff_t>(size));
        }
    }

    void SocketsHost::drop(Peer &peer)
    {
        closeSocket(peer.handle);

        peer.handle = kNoHandle;
    }

    std::vector<Packet> SocketsHost::receive()
    {
        return std::exchange(arrived, {});
    }

    std::vector<PeerId> SocketsHost::peers() const
    {
        std::vector<PeerId> ids;
        ids.reserve(links.size());

        for (const Peer &peer : links)
        {
            if (peer.up)
            {
                ids.push_back(peer.id);
            }
        }

        return ids;
    }

    SocketsHost::Peer *SocketsHost::find(PeerId peer)
    {
        for (Peer &one : links)
        {
            if (one.id == peer && one.up)
            {
                return &one;
            }
        }

        return nullptr;
    }

}
