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

    /**
     * @brief The state every host of one backend shares.
     *
     * Declared here and defined privately, which is the pimpl this
     * class is: a caller needs a LoopbackBackend without needing to
     * know how two hosts in one process find each other.
     */
    class LoopbackNetwork;

    /** @brief How many peers one loopback host may hold at once. */
    inline constexpr std::size_t kLoopbackMaxPeers = 16;

    /** @brief The largest payload a loopback send will carry. */
    inline constexpr std::size_t kLoopbackMaxPayloadBytes = 65536;

    /**
     * @brief Every host in one process, wired to each other.
     *
     * **This is what makes real multiplayer a unit test.** Two peers, a
     * scripted delay and a scripted list of packets to throw away, all
     * inside one process with no socket, no port and no wall-clock time
     * spent -- which is sound::OfflineDevice's arrangement applied to a
     * network.
     *
     * It lives here rather than under backends/ for NullNetworkBackend's
     * reason: `backends/` is exempt from the coverage gate and `src/` is
     * not, so an in-process implementation kept here is one the gate can
     * hold to 100%.
     * It is deliberately *not* selectable as ANTWIKA_NETWORK_BACKEND
     * either, since a build asking for a transport wants one that leaves
     * the process.
     *
     * **A host may not outlive the backend that opened it.** The hosts
     * find each other through state this owns, so the usual rule about
     * a borrowed collaborator applies to the thing that handed the
     * borrow out.
     */
    class LoopbackBackend final : public INetworkBackend
    {
    public:
        /**
         * @brief Construct the backend over what it does to packets.
         * @param logger Receives its diagnostics; must outlive it.
         * @param schedule What happens to packets crossing it; the
         * default delivers everything on the next pump.
         */
        explicit LoopbackBackend(
            ILogger &logger, DeliverySchedule schedule = {});

        LoopbackBackend(const LoopbackBackend &) = delete;
        LoopbackBackend(LoopbackBackend &&) = delete;

        LoopbackBackend &operator=(const LoopbackBackend &) = delete;
        LoopbackBackend &operator=(LoopbackBackend &&) = delete;

        /**
         * @brief Tear the network down.
         *
         * Declared here and defined where the state it owns is a
         * complete type, which is what a pimpl owner does.
         */
        ~LoopbackBackend() override;

        [[nodiscard]] std::string_view name() const override;

        [[nodiscard]] NetworkCapabilities capabilities() const override;

        [[nodiscard]] std::unique_ptr<IHost> openHost(
            const Endpoint &endpoint) override;

    private:
        ILogger &logger;
        std::unique_ptr<LoopbackNetwork> network;
    };

} // namespace antwika::network
