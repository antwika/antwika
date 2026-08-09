#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

#include <antwika/log/mocks/MockLogger.hpp>
#include <antwika/network/Endpoint.hpp>
#include <antwika/network/IHost.hpp>
#include <antwika/network/NetworkError.hpp>
#include <antwika/network/Packet.hpp>
#include <antwika/network/PeerId.hpp>
#include <antwika/network/Port.hpp>

#include "SocketsHost.hpp"
#include "SocketsBackend.hpp"

using antwika::log::mocks::MockLogger;
using antwika::network::Endpoint;
using antwika::network::IHost;
using antwika::network::NetworkError;
using antwika::network::Packet;
using antwika::network::PeerId;
using antwika::network::Port;
using antwika::network::rawValue;
using antwika::network::sockets::kMaxPayloadBytes;
using antwika::network::sockets::SocketsBackend;
using ::testing::NiceMock;

namespace
{
    constexpr int kPatience = 200;

    Endpoint anywhere()
    {
        return Endpoint{.host = "127.0.0.1", .port = Port{0}};
    }

    std::vector<std::byte> payloadOf(std::size_t size, unsigned char seed)
    {
        std::vector<std::byte> out;
        out.reserve(size);

        for (std::size_t index = 0; index < size; ++index)
        {
            out.push_back(
                static_cast<std::byte>((index + seed) & 0xFFU));
        }

        return out;
    }

    void settle(IHost &left, IHost &right)
    {
        for (int pump = 0; pump < kPatience; ++pump)
        {
            left.pump();
            right.pump();
        }
    }

    std::vector<Packet> awaited(IHost &host, std::size_t wanted)
    {
        std::vector<Packet> all;

        for (int pump = 0; pump < kPatience && all.size() < wanted;
             ++pump)
        {
            host.pump();

            auto some = host.receive();

            all.insert(all.end(), some.begin(), some.end());
        }

        return all;
    }

    class SocketsHostTest : public ::testing::Test
    {
    protected:
        NiceMock<MockLogger> logger;
        SocketsBackend backend{logger};
    };
}

TEST_F(SocketsHostTest, Endpoint_ReportsThePortTheSystemChose)
{
    const auto host = backend.openHost(anywhere());

    EXPECT_EQ(host->endpoint().host, "127.0.0.1");
    EXPECT_NE(rawValue(host->endpoint().port), 0U);
}

TEST_F(SocketsHostTest, Endpoint_DiffersBetweenTwoHosts)
{
    const auto left = backend.openHost(anywhere());
    const auto right = backend.openHost(anywhere());

    EXPECT_NE(left->endpoint().port, right->endpoint().port);
}

TEST_F(SocketsHostTest, OpenHost_RefusesAHostNameThatResolvesToNothing)
{
    EXPECT_THROW(
        (void)backend.openHost(
            Endpoint{
                .host = "antwika.invalid.example",
                .port = Port{0}}),
        NetworkError);
}

TEST_F(SocketsHostTest, Send_ReassemblesAPayloadLargerThanOneRead)
{
    const auto left = backend.openHost(anywhere());
    const auto right = backend.openHost(anywhere());
    const PeerId peer = left->connect(right->endpoint());
    settle(*left, *right);

    const auto big = payloadOf(kMaxPayloadBytes, 3);

    left->send(peer, big);

    for (int pump = 0; pump < kPatience; ++pump)
    {
        left->pump();
    }

    const auto arrived = awaited(*right, 1);

    ASSERT_EQ(arrived.size(), 1U);
    EXPECT_EQ(arrived.front().payload, big);
}

TEST_F(SocketsHostTest, Send_KeepsManySmallMessagesApartAndInOrder)
{
    const auto left = backend.openHost(anywhere());
    const auto right = backend.openHost(anywhere());
    const PeerId peer = left->connect(right->endpoint());
    settle(*left, *right);

    for (unsigned char index = 0; index < 16; ++index)
    {
        left->send(peer, payloadOf(4, index));
    }

    for (int pump = 0; pump < kPatience; ++pump)
    {
        left->pump();
    }

    const auto arrived = awaited(*right, 16);

    ASSERT_EQ(arrived.size(), 16U);

    for (unsigned char index = 0; index < 16; ++index)
    {
        EXPECT_EQ(arrived[index].payload, payloadOf(4, index));
    }
}

TEST_F(SocketsHostTest, Connect_ToARefusedPortNeverBecomesAPeer)
{
    const auto host = backend.openHost(anywhere());

    (void)host->connect(Endpoint{.host = "127.0.0.1", .port = Port{1}});

    for (int pump = 0; pump < kPatience; ++pump)
    {
        host->pump();
    }

    EXPECT_TRUE(host->peers().empty());
}

TEST_F(SocketsHostTest, Send_CarriesBothWaysOverOneLink)
{
    const auto left = backend.openHost(anywhere());
    const auto right = backend.openHost(anywhere());
    const PeerId there = left->connect(right->endpoint());
    settle(*left, *right);

    ASSERT_EQ(right->peers().size(), 1U);

    const PeerId back = right->peers().front();

    left->send(there, payloadOf(8, 1));
    right->send(back, payloadOf(8, 2));
    settle(*left, *right);

    const auto atRight = awaited(*right, 1);
    const auto atLeft = awaited(*left, 1);

    ASSERT_EQ(atRight.size(), 1U);
    ASSERT_EQ(atLeft.size(), 1U);
    EXPECT_EQ(atRight.front().payload, payloadOf(8, 1));
    EXPECT_EQ(atLeft.front().payload, payloadOf(8, 2));
}
