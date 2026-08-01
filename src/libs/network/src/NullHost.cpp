#include "antwika/network/NullHost.hpp"

#include <utility>

#include "antwika/network/NetworkError.hpp"

namespace antwika::network
{

    NullHost::NullHost(Endpoint endpoint) : own(std::move(endpoint))
    {
    }

    Endpoint NullHost::endpoint() const
    {
        return own;
    }

    PeerId NullHost::connect(const Endpoint &remote)
    {
        throw NetworkError(
            "antwika::network: the null backend cannot connect to "
            + remote.host);
    }

    void NullHost::disconnect(PeerId)
    {
    }

    void NullHost::send(PeerId, std::span<const std::byte>)
    {
    }

    void NullHost::broadcast(std::span<const std::byte>)
    {
    }

    void NullHost::pump()
    {
    }

    std::vector<Packet> NullHost::receive()
    {
        return {};
    }

    std::vector<PeerId> NullHost::peers() const
    {
        return {};
    }

} // namespace antwika::network
