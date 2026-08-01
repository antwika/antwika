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

    /**
     * @brief Hosts that leave the process, over TCP.
     *
     * The first backend in this tree that is **not a framework**: it
     * names the operating system's own socket API and adds no package,
     * no lockfile entry and no process-global event queue.
     * That is why it is exempt from the one-real-framework rule
     * backends/CMakeLists.txt applies to the other three subsystems --
     * the two reasons that rule gives, a shared event queue and a
     * doubled dependency graph, are both absent here.
     */
    class SocketsBackend final : public INetworkBackend
    {
    public:
        /**
         * @brief Construct the backend over where it reports to.
         * @param logger Receives its diagnostics; must outlive it.
         * @throws NetworkError If the platform's socket library refused
         * to start, which only Winsock can do.
         */
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

} // namespace antwika::network::sockets
