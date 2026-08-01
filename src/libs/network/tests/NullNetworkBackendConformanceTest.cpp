#include <cstdint>
#include <memory>
#include <string>

#include <antwika/network/conformance/NetworkBackendConformance.hpp>

#include "antwika/network/NullNetworkBackend.hpp"

namespace antwika::network::conformance
{

    namespace
    {
        /**
         * @brief Builds a NullNetworkBackend for the shared suite.
         */
        struct NullNetworkBackendTraits
        {
            static std::unique_ptr<INetworkBackend> create(ILogger &logger)
            {
                return std::make_unique<NullNetworkBackend>(logger);
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
        Null,
        NetworkBackendConformance,
        NullNetworkBackendTraits);

} // namespace antwika::network::conformance
