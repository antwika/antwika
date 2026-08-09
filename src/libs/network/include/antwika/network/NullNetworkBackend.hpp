#pragma once

#include <memory>
#include <string_view>

#include <antwika/log/ILogger.hpp>

#include "antwika/network/Endpoint.hpp"
#include "antwika/network/IHost.hpp"
#include "antwika/network/INetworkBackend.hpp"
#include "antwika/network/NetworkCapabilities.hpp"

namespace antwika::network
{

    using antwika::log::ILogger;

    class NullNetworkBackend final : public INetworkBackend
    {
    public:
        explicit NullNetworkBackend(ILogger &logger);

        NullNetworkBackend(const NullNetworkBackend &) = delete;
        NullNetworkBackend(NullNetworkBackend &&) = delete;

        NullNetworkBackend &operator=(const NullNetworkBackend &) = delete;
        NullNetworkBackend &operator=(NullNetworkBackend &&) = delete;

        [[nodiscard]] std::string_view name() const override;

        [[nodiscard]] NetworkCapabilities capabilities() const override;

        [[nodiscard]] std::unique_ptr<IHost> openHost(
            const Endpoint &endpoint) override;

    private:
        ILogger &logger;
    };

}
