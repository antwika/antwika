#include <memory>
#include <string>

#include <antwika/network/conformance/NetworkBackendConformanceTest.hpp>

#include "SocketsBackend.hpp"

namespace antwika::network::conformance
{

    namespace
    {
        struct SocketsBackendTraits final
        {
            static std::unique_ptr<INetworkBackend> create(ILogger &logger)
            {
                return std::make_unique<sockets::SocketsBackend>(logger);
            }

            static Endpoint endpointFor(unsigned)
            {
                return Endpoint{.host = "127.0.0.1", .port = Port{0}};
            }

            static Endpoint nowhere()
            {
                return Endpoint{.host = "127.0.0.1", .port = Port{1}};
            }
        };
    }

    INSTANTIATE_TYPED_TEST_SUITE_P(
        Sockets,
        NetworkBackendConformanceTest,
        SocketsBackendTraits);

}
