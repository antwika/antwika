#include "antwika/network/SelectedNetworkBackend.hpp"

#include <memory>

#include "antwika/network/NullNetworkBackend.hpp"

namespace antwika::network
{

    std::unique_ptr<INetworkBackend> makeSelectedNetworkBackend(
        ILogger &logger)
    {
        return std::make_unique<NullNetworkBackend>(logger);
    }

} // namespace antwika::network
