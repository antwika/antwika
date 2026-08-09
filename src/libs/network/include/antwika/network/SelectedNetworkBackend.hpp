#pragma once

#include <memory>

#include <antwika/log/ILogger.hpp>

#include "antwika/network/INetworkBackend.hpp"

namespace antwika::network
{

    using antwika::log::ILogger;

    [[nodiscard]] std::unique_ptr<INetworkBackend>
    makeSelectedNetworkBackend(ILogger &logger);

}
