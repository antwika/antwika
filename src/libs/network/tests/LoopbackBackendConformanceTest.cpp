#include <cstdint>
#include <memory>
#include <string>

#include <antwika/network/conformance/NetworkBackendConformance.hpp>

#include "antwika/network/LoopbackBackend.hpp"

namespace antwika::network::conformance
{

    namespace
    {
        /**
         * @brief Builds a LoopbackBackend for the shared suite.
         *
         * The default schedule, deliberately: this run is about whether
         * the backend keeps INetworkBackend's promises, and a scripted
         * delay or a dropped packet is a promise about a *network*
         * rather than about a backend -- LoopbackBackendTest is where
         * those belong.
         */
        struct LoopbackBackendTraits
        {
            static std::unique_ptr<INetworkBackend> create(ILogger &logger)
            {
                return std::make_unique<LoopbackBackend>(logger);
            }

            static Endpoint endpointFor(unsigned index)
            {
                return Endpoint{
                    .host = "localhost",
                    .port = Port{static_cast<std::uint16_t>(9000 + index)}};
            }

            // A port no host of this run is ever opened at.
            static Endpoint nowhere()
            {
                return Endpoint{.host = "localhost", .port = Port{9999}};
            }
        };
    } // namespace

    INSTANTIATE_TYPED_TEST_SUITE_P(
        Loopback,
        NetworkBackendConformance,
        LoopbackBackendTraits);

} // namespace antwika::network::conformance
