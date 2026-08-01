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

    /**
     * @brief A backend whose hosts talk to nobody.
     *
     * It lives here rather than under backends/null/ for the reason
     * sound::NullSoundBackend and input::NullInputBackend do:
     * `backends/` is exempt from the coverage gate and `src/` is not,
     * so a headless implementation kept here is one the gate can
     * actually hold to 100%.
     * All that lives under backends/null/ is the two-line factory.
     */
    class NullNetworkBackend final : public INetworkBackend
    {
    public:
        /**
         * @brief Construct the backend over where it reports to.
         * @param logger Receives its diagnostics; must outlive it.
         */
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

} // namespace antwika::network
