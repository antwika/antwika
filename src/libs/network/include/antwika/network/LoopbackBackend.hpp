#pragma once

#include <cstddef>
#include <memory>
#include <string_view>

#include <antwika/log/ILogger.hpp>

#include "antwika/network/DeliverySchedule.hpp"
#include "antwika/network/Endpoint.hpp"
#include "antwika/network/IHost.hpp"
#include "antwika/network/INetworkBackend.hpp"
#include "antwika/network/NetworkCapabilities.hpp"

namespace antwika::network
{

    using antwika::log::ILogger;

    class LoopbackNetwork;

    inline constexpr std::size_t kLoopbackMaxPeers = 16;

    inline constexpr std::size_t kLoopbackMaxPayloadBytes = 65536;

    class LoopbackBackend final : public INetworkBackend
    {
    public:
        explicit LoopbackBackend(
            ILogger &logger, DeliverySchedule schedule = {});

        LoopbackBackend(const LoopbackBackend &) = delete;
        LoopbackBackend(LoopbackBackend &&) = delete;

        LoopbackBackend &operator=(const LoopbackBackend &) = delete;
        LoopbackBackend &operator=(LoopbackBackend &&) = delete;

        ~LoopbackBackend() override;

        [[nodiscard]] std::string_view name() const override;

        [[nodiscard]] NetworkCapabilities capabilities() const override;

        [[nodiscard]] std::unique_ptr<IHost> openHost(
            const Endpoint &endpoint) override;

    private:
        ILogger &logger;
        std::unique_ptr<LoopbackNetwork> network;
    };

}
