#pragma once

#include <memory>
#include <string_view>

#include <antwika/log/ILogger.hpp>
#include <antwika/network/Endpoint.hpp>
#include <antwika/network/IHost.hpp>
#include <antwika/network/INetworkBackend.hpp>
#include <antwika/network/NetworkCapabilities.hpp>

namespace antwika::network::sockets
{

    using antwika::log::ILogger;

    class SocketsBackend final : public INetworkBackend
    {
    public:
        explicit SocketsBackend(ILogger &logger);

        SocketsBackend(const SocketsBackend &) = delete;
        SocketsBackend(SocketsBackend &&) = delete;

        SocketsBackend &operator=(const SocketsBackend &) = delete;
        SocketsBackend &operator=(SocketsBackend &&) = delete;

        [[nodiscard]] std::string_view name() const override;

        [[nodiscard]] NetworkCapabilities capabilities() const override;

        [[nodiscard]] std::unique_ptr<IHost> openHost(
            const Endpoint &endpoint) override;

    private:
        ILogger &logger;
    };

}
