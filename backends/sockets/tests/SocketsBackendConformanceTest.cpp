#include <memory>
#include <string>

#include <antwika/network/conformance/NetworkBackendConformance.hpp>

#include "SocketsBackend.hpp"

namespace antwika::network::conformance
{

    namespace
    {
        /**
         * @brief Builds a SocketsBackend for the shared suite.
         *
         * **Every host asks for port 0**, so the operating system picks
         * one and no two runs of this suite can collide -- which
         * matters because CTest runs each case in a process of its own,
         * several at a time.
         * The suite dials whatever a host reports rather than what it
         * was asked for, which is the same thing for the in-process
         * backends and the only workable thing here.
         */
        struct SocketsBackendTraits
        {
            static std::unique_ptr<INetworkBackend> create(ILogger &logger)
            {
                return std::make_unique<sockets::SocketsBackend>(logger);
            }

            static Endpoint endpointFor(unsigned)
            {
                return Endpoint{.host = "127.0.0.1", .port = Port{0}};
            }

            // Nothing may listen here.
            // Binding a privileged port needs a privilege tests lack.
            static Endpoint nowhere()
            {
                return Endpoint{.host = "127.0.0.1", .port = Port{1}};
            }
        };
    } // namespace

    INSTANTIATE_TYPED_TEST_SUITE_P(
        Sockets,
        NetworkBackendConformance,
        SocketsBackendTraits);

} // namespace antwika::network::conformance
