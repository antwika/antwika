#include <cstdint>
#include <memory>
#include <string>

#include <antwika/network/conformance/NetworkBackendConformanceTest.hpp>

#include "antwika/network/LoopbackBackend.hpp"

namespace antwika::network::conformance
{

    namespace
    {
        struct LoopbackBackendTraits final
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

            static Endpoint nowhere()
            {
                return Endpoint{.host = "localhost", .port = Port{9999}};
            }
        };
    }

    INSTANTIATE_TYPED_TEST_SUITE_P(
        Loopback,
        NetworkBackendConformanceTest,
        LoopbackBackendTraits);

}
