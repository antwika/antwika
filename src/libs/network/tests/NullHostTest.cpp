#include <gtest/gtest.h>

#include <array>
#include <cstddef>

#include "antwika/network/NullHost.hpp"
#include "antwika/network/Endpoint.hpp"
#include "antwika/network/NetworkError.hpp"
#include "antwika/network/PeerId.hpp"
#include "antwika/network/Port.hpp"

using antwika::network::Endpoint;
using antwika::network::NetworkError;
using antwika::network::NullHost;
using antwika::network::PeerId;
using antwika::network::Port;

namespace
{
    constexpr Endpoint kHere{.host = "localhost", .port = Port{9000}};
    constexpr std::array<std::byte, 2> kPayload{
        std::byte{1}, std::byte{2}};
}

TEST(NullHostTest, Endpoint_ReportsWhereItWasOpened)
{
    const NullHost host{Endpoint{kHere}};

    EXPECT_EQ(host.endpoint(), kHere);
}

TEST(NullHostTest, Connect_IsRefusedAndNamesWhoItCouldNotReach)
{
    NullHost host{Endpoint{kHere}};

    EXPECT_THROW(
        {
            try
            {
                (void)host.connect(
                    Endpoint{.host = "elsewhere", .port = Port{1}});
            }
            catch (const NetworkError &error)
            {
                EXPECT_NE(
                    std::string(error.what()).find("elsewhere"),
                    std::string::npos);
                throw;
            }
        },
        NetworkError);
}

TEST(NullHostTest, Peers_IsEmptyBecauseItTalksToNobody)
{
    const NullHost host{Endpoint{kHere}};

    EXPECT_TRUE(host.peers().empty());
}

TEST(NullHostTest, Receive_IsEmptyHoweverOftenItIsPumped)
{
    NullHost host{Endpoint{kHere}};

    host.pump();
    host.pump();

    EXPECT_TRUE(host.receive().empty());
}

TEST(NullHostTest, Send_AndDisconnectDoNothingAtAll)
{
    NullHost host{Endpoint{kHere}};

    host.send(PeerId{1}, kPayload);
    host.broadcast(kPayload);
    host.disconnect(PeerId{1});
    host.pump();

    EXPECT_TRUE(host.peers().empty());
    EXPECT_TRUE(host.receive().empty());
}
