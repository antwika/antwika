#include "antwika/network/SelectedNetworkBackend.hpp"

#include <memory>

#include "SocketsBackend.hpp"

namespace antwika::network
{

    std::unique_ptr<INetworkBackend> makeSelectedNetworkBackend(
        ILogger &logger)
    {
        return std::make_unique<sockets::SocketsBackend>(logger);
    }

} // namespace antwika::network
