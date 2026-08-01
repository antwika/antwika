#pragma once

#include <memory>

#include <antwika/log/ILogger.hpp>

#include "antwika/network/INetworkBackend.hpp"

namespace antwika::network
{

    using antwika::log::ILogger;

    /**
     * @brief Create the network backend chosen at build time.
     *
     * Declared here but deliberately not defined here: the definition
     * comes from whichever backend under backends/ was selected via
     * ANTWIKA_NETWORK_BACKEND. That is what keeps every socket API out
     * of src/ entirely. Linking a program that calls this without
     * linking a backend is a link error, by design.
     *
     * @param logger Receives the backend's diagnostics.
     * @return The selected backend, never null.
     * @throws NetworkError If the underlying transport failed to
     * initialise.
     */
    [[nodiscard]] std::unique_ptr<INetworkBackend>
    makeSelectedNetworkBackend(ILogger &logger);

} // namespace antwika::network
