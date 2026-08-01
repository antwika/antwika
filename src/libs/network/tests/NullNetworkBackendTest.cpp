#include "antwika/network/NullNetworkBackend.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <antwika/log/mocks/MockLogger.hpp>

#include "antwika/network/Endpoint.hpp"
#include "antwika/network/NetworkCapabilities.hpp"
#include "antwika/network/NetworkError.hpp"
#include "antwika/network/Port.hpp"

using antwika::log::mocks::MockLogger;
using antwika::network::Endpoint;
using antwika::network::NetworkCapabilities;
using antwika::network::NetworkError;
using antwika::network::NullNetworkBackend;
using antwika::network::Port;
using ::testing::NiceMock;

namespace
{
    constexpr Endpoint kHere{.host = "localhost", .port = Port{9000}};
} // namespace

TEST(NullNetworkBackendTest, IsCalledNull)
{
    NiceMock<MockLogger> logger;
    const NullNetworkBackend backend(logger);

    EXPECT_EQ(backend.name(), "null");
}

// Saying so is what lets the conformance suite skip honestly.
TEST(NullNetworkBackendTest, ReachesNobodyAndSaysSo)
{
    NiceMock<MockLogger> logger;
    const NullNetworkBackend backend(logger);

    EXPECT_EQ(
        backend.capabilities(),
        (NetworkCapabilities{
            .connects = false,
            .listens = false,
            .maxPeers = 0,
            .maxPayloadBytes = 0}));
}

TEST(NullNetworkBackendTest, OpenHost_ReportsTheEndpointItWasOpenedAt)
{
    NiceMock<MockLogger> logger;
    NullNetworkBackend backend(logger);

    const auto host = backend.openHost(kHere);

    ASSERT_NE(host, nullptr);
    EXPECT_EQ(host->endpoint(), kHere);
}

// A backend that reaches nobody still answers as a real one does.
TEST(NullNetworkBackendTest, OpenHost_RefusesAnEndpointWithNoHostName)
{
    NiceMock<MockLogger> logger;
    NullNetworkBackend backend(logger);

    EXPECT_THROW(
        {
            try
            {
                (void)backend.openHost(
                    Endpoint{.host = "", .port = Port{9000}});
            }
            catch (const NetworkError &error)
            {
                EXPECT_NE(
                    std::string(error.what()).find("9000"),
                    std::string::npos);
                throw;
            }
        },
        NetworkError);
}
