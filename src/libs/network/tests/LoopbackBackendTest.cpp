#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include <antwika/log/mocks/MockLogger.hpp>

#include "antwika/network/LoopbackBackend.hpp"
#include "antwika/network/DeliverySchedule.hpp"
#include "antwika/network/Endpoint.hpp"
#include "antwika/network/IHost.hpp"
#include "antwika/network/NetworkCapabilities.hpp"
#include "antwika/network/NetworkError.hpp"
#include "antwika/network/Packet.hpp"
#include "antwika/network/PeerId.hpp"
#include "antwika/network/Port.hpp"

using antwika::log::mocks::MockLogger;
using antwika::network::DeliverySchedule;
using antwika::network::Endpoint;
using antwika::network::IHost;
using antwika::network::kLoopbackMaxPayloadBytes;
using antwika::network::kLoopbackMaxPeers;
using antwika::network::LoopbackBackend;
using antwika::network::NetworkCapabilities;
using antwika::network::NetworkError;
using antwika::network::Packet;
using antwika::network::PeerId;
using antwika::network::Port;
using antwika::network::rawValue;
using ::testing::NiceMock;

namespace
{
    Endpoint at(std::uint16_t port)
    {
        return Endpoint{.host = "localhost", .port = Port{port}};
    }

    std::vector<std::byte> bytes(std::string_view text)
    {
        std::vector<std::byte> out;
        out.reserve(text.size());

        for (const char letter : text)
        {
            out.push_back(static_cast<std::byte>(letter));
        }

        return out;
    }

    std::vector<Packet> exchange(
        IHost &from, PeerId peer, std::string_view text, IHost &to)
    {
        from.send(peer, bytes(text));
        from.pump();
        to.pump();

        return to.receive();
    }

    class LoopbackBackendTest : public ::testing::Test
    {
    protected:
        NiceMock<MockLogger> logger;
        LoopbackBackend backend{logger};
    };
}

TEST_F(LoopbackBackendTest, Name_IsLoopback)
{
    EXPECT_EQ(backend.name(), "loopback");
}

TEST_F(LoopbackBackendTest, Capabilities_SayItConnectsAndListens)
{
    EXPECT_EQ(
        backend.capabilities(),
        (NetworkCapabilities{
            .connects = true,
            .listens = true,
            .maxPeers = kLoopbackMaxPeers,
            .maxPayloadBytes = kLoopbackMaxPayloadBytes}));

    EXPECT_EQ(backend.capabilities().maxPeers, 16U);
    EXPECT_EQ(backend.capabilities().maxPayloadBytes, 65536U);
}

TEST_F(LoopbackBackendTest, OpenHost_ReportsTheEndpointItWasOpenedAt)
{
    const auto host = backend.openHost(at(1));

    ASSERT_NE(host, nullptr);
    EXPECT_EQ(host->endpoint(), at(1));
    EXPECT_TRUE(host->peers().empty());
}

TEST_F(LoopbackBackendTest, OpenHost_RefusesAnEndpointWithNoHostName)
{
    EXPECT_THROW(
        (void)backend.openHost(Endpoint{.host = "", .port = Port{1}}),
        NetworkError);
}

TEST_F(LoopbackBackendTest, OpenHost_RefusesASecondHostAtOneEndpoint)
{
    const auto first = backend.openHost(at(1));

    EXPECT_THROW((void)backend.openHost(at(1)), NetworkError);
}

TEST_F(LoopbackBackendTest, OpenHost_ReusesAnEndpointOnceItsHostHasGone)
{
    { const auto first = backend.openHost(at(1)); }

    const auto second = backend.openHost(at(1));

    EXPECT_EQ(second->endpoint(), at(1));
}

TEST_F(LoopbackBackendTest, Connect_LinksBothHostsToEachOther)
{
    const auto left = backend.openHost(at(1));
    const auto right = backend.openHost(at(2));

    const PeerId peer = left->connect(at(2));

    EXPECT_EQ(left->peers(), std::vector<PeerId>{peer});
    EXPECT_EQ(right->peers().size(), 1U);
}

TEST_F(LoopbackBackendTest, Connect_ToNobodyNeverBecomesAPeer)
{
    const auto host = backend.openHost(at(1));

    const PeerId peer = host->connect(at(99));

    host->pump();

    EXPECT_TRUE(host->peers().empty());
    EXPECT_NE(rawValue(peer), 0U);
}

TEST_F(LoopbackBackendTest, Connect_ToNobodySpendsAName)
{
    const auto host = backend.openHost(at(1));
    const auto other = backend.openHost(at(2));

    const PeerId nowhere = host->connect(at(99));
    const PeerId real = host->connect(at(2));

    EXPECT_NE(nowhere, real);
    EXPECT_EQ(host->peers(), std::vector<PeerId>{real});
}

TEST_F(LoopbackBackendTest, Connect_RefusesAHostConnectingToItself)
{
    const auto host = backend.openHost(at(1));

    EXPECT_THROW((void)host->connect(at(1)), NetworkError);
}

TEST_F(LoopbackBackendTest, Connect_RefusesOnceThisHostHoldsItsLimit)
{
    const auto host = backend.openHost(at(1));
    std::vector<std::unique_ptr<IHost>> others;

    for (std::uint16_t index = 0; index < kLoopbackMaxPeers + 1; ++index)
    {
        others.push_back(
            backend.openHost(at(static_cast<std::uint16_t>(index + 2))));
    }

    for (std::uint16_t index = 0; index < kLoopbackMaxPeers; ++index)
    {
        (void)host->connect(at(static_cast<std::uint16_t>(index + 2)));
    }

    EXPECT_EQ(host->peers().size(), kLoopbackMaxPeers);
    EXPECT_THROW(
        (void)host->connect(
            at(static_cast<std::uint16_t>(kLoopbackMaxPeers + 2))),
        NetworkError);
}

TEST_F(LoopbackBackendTest, Connect_RefusesOnceTheOtherHostHoldsItsLimit)
{
    const auto busy = backend.openHost(at(1));
    const auto host = backend.openHost(at(2));
    std::vector<std::unique_ptr<IHost>> others;

    for (std::uint16_t index = 0; index < kLoopbackMaxPeers; ++index)
    {
        others.push_back(
            backend.openHost(at(static_cast<std::uint16_t>(index + 3))));
        (void)others.back()->connect(at(1));
    }

    EXPECT_TRUE(host->peers().empty());
    EXPECT_THROW((void)host->connect(at(1)), NetworkError);
}

TEST_F(LoopbackBackendTest, Send_ArrivesAtTheOtherHostOnTheNextPump)
{
    const auto left = backend.openHost(at(1));
    const auto right = backend.openHost(at(2));
    const PeerId peer = left->connect(at(2));

    const auto arrived = exchange(*left, peer, "hello", *right);

    ASSERT_EQ(arrived.size(), 1U);
    EXPECT_EQ(arrived.front().payload, bytes("hello"));
}

TEST_F(LoopbackBackendTest, Send_NamesTheSenderAsTheReceiverNamesIt)
{
    const auto left = backend.openHost(at(1));
    const auto right = backend.openHost(at(2));
    (void)left->connect(at(2));

    const PeerId asRightNamesLeft = right->peers().front();
    const PeerId asLeftNamesRight = left->peers().front();

    const auto arrived =
        exchange(*left, asLeftNamesRight, "hello", *right);

    ASSERT_EQ(arrived.size(), 1U);
    EXPECT_EQ(arrived.front().from, asRightNamesLeft);
}

TEST_F(LoopbackBackendTest, Send_IsNotDeliveredUntilTheHostIsPumped)
{
    const auto left = backend.openHost(at(1));
    const auto right = backend.openHost(at(2));
    const PeerId peer = left->connect(at(2));

    left->send(peer, bytes("hello"));
    right->pump();

    EXPECT_TRUE(right->receive().empty());

    left->pump();
    right->pump();

    EXPECT_EQ(right->receive().size(), 1U);
}

TEST_F(LoopbackBackendTest, Send_ToAPeerThatIsNotHeldDoesNothing)
{
    const auto left = backend.openHost(at(1));
    const auto right = backend.openHost(at(2));
    (void)left->connect(at(2));

    left->send(PeerId{99}, bytes("hello"));
    left->pump();
    right->pump();

    EXPECT_TRUE(right->receive().empty());
}

TEST_F(LoopbackBackendTest, Send_RefusesAPayloadLargerThanItCarries)
{
    const auto left = backend.openHost(at(1));
    const auto right = backend.openHost(at(2));
    const PeerId peer = left->connect(at(2));
    const std::vector<std::byte> huge(kLoopbackMaxPayloadBytes + 1);

    EXPECT_THROW(left->send(peer, huge), NetworkError);
    EXPECT_THROW(left->broadcast(huge), NetworkError);
}

TEST_F(LoopbackBackendTest, Send_CarriesAPayloadOfExactlyTheLimit)
{
    const auto left = backend.openHost(at(1));
    const auto right = backend.openHost(at(2));
    const PeerId peer = left->connect(at(2));
    const std::vector<std::byte> full(kLoopbackMaxPayloadBytes);

    left->send(peer, full);
    left->pump();
    right->pump();

    const auto arrived = right->receive();

    ASSERT_EQ(arrived.size(), 1U);
    EXPECT_EQ(arrived.front().payload.size(), kLoopbackMaxPayloadBytes);
}

TEST_F(LoopbackBackendTest, Broadcast_ReachesEveryPeerAndNobodyElse)
{
    const auto left = backend.openHost(at(1));
    const auto first = backend.openHost(at(2));
    const auto second = backend.openHost(at(3));
    const auto stranger = backend.openHost(at(4));
    (void)left->connect(at(2));
    (void)left->connect(at(3));

    left->broadcast(bytes("all"));
    left->pump();
    first->pump();
    second->pump();
    stranger->pump();

    EXPECT_EQ(first->receive().size(), 1U);
    EXPECT_EQ(second->receive().size(), 1U);
    EXPECT_TRUE(stranger->receive().empty());
}

TEST_F(LoopbackBackendTest, Broadcast_WithNoPeersDoesNothing)
{
    const auto host = backend.openHost(at(1));

    host->broadcast(bytes("all"));
    host->pump();
    host->pump();

    EXPECT_TRUE(host->receive().empty());
}

TEST_F(LoopbackBackendTest, Receive_HandsOverEveryPacketInFlightAtOnce)
{
    const auto left = backend.openHost(at(1));
    const auto right = backend.openHost(at(2));
    const PeerId peer = left->connect(at(2));

    left->send(peer, bytes("first"));
    left->send(peer, bytes("second"));
    left->pump();
    right->pump();

    const auto arrived = right->receive();

    ASSERT_EQ(arrived.size(), 2U);
    EXPECT_EQ(arrived.front().payload, bytes("first"));
    EXPECT_EQ(arrived.back().payload, bytes("second"));
}

TEST_F(LoopbackBackendTest, Receive_HandsEachPacketOverOnlyOnce)
{
    const auto left = backend.openHost(at(1));
    const auto right = backend.openHost(at(2));
    const PeerId peer = left->connect(at(2));

    EXPECT_EQ(exchange(*left, peer, "hello", *right).size(), 1U);
    EXPECT_TRUE(right->receive().empty());
}

TEST(LoopbackScheduleTest, Pump_HoldsAPacketForTheScheduledDelay)
{
    NiceMock<MockLogger> logger;
    LoopbackBackend backend(
        logger, DeliverySchedule{.delayPumps = 2, .dropped = {}});
    const auto left = backend.openHost(at(1));
    const auto right = backend.openHost(at(2));
    const PeerId peer = left->connect(at(2));

    left->send(peer, bytes("late"));
    left->pump();

    right->pump();
    EXPECT_TRUE(right->receive().empty());

    right->pump();
    EXPECT_TRUE(right->receive().empty());

    right->pump();
    EXPECT_EQ(right->receive().size(), 1U);
}

TEST(LoopbackScheduleTest, Pump_ThrowsAwayTheScheduledSends)
{
    NiceMock<MockLogger> logger;
    LoopbackBackend backend(
        logger, DeliverySchedule{.delayPumps = 0, .dropped = {1}});
    const auto left = backend.openHost(at(1));
    const auto right = backend.openHost(at(2));
    const PeerId peer = left->connect(at(2));

    EXPECT_EQ(exchange(*left, peer, "first", *right).size(), 1U);
    EXPECT_TRUE(exchange(*left, peer, "second", *right).empty());
    EXPECT_EQ(exchange(*left, peer, "third", *right).size(), 1U);
}

TEST_F(LoopbackBackendTest, Disconnect_TakesThePeerOffBothHosts)
{
    const auto left = backend.openHost(at(1));
    const auto right = backend.openHost(at(2));
    const PeerId peer = left->connect(at(2));

    left->disconnect(peer);

    EXPECT_TRUE(left->peers().empty());
    EXPECT_TRUE(right->peers().empty());
}

TEST_F(LoopbackBackendTest, Disconnect_DropsWhatWasStillQueuedForThatPeer)
{
    const auto left = backend.openHost(at(1));
    const auto right = backend.openHost(at(2));
    const PeerId peer = left->connect(at(2));

    left->send(peer, bytes("undelivered"));
    left->disconnect(peer);
    left->pump();
    right->pump();

    EXPECT_TRUE(right->receive().empty());
}

TEST_F(LoopbackBackendTest, Closing_DropsWhatWasStillQueuedForThatHost)
{
    const auto left = backend.openHost(at(1));

    {
        const auto right = backend.openHost(at(2));
        const PeerId peer = left->connect(at(2));

        left->send(peer, bytes("undelivered"));
    }

    left->pump();

    EXPECT_TRUE(left->peers().empty());
}

TEST_F(LoopbackBackendTest, Disconnect_IsSafeForAPeerThatIsNotHeld)
{
    const auto host = backend.openHost(at(1));

    host->disconnect(PeerId{99});

    EXPECT_TRUE(host->peers().empty());
}

TEST_F(LoopbackBackendTest, Closing_TakesTheHostOffEveryPeer)
{
    const auto right = backend.openHost(at(2));

    {
        const auto left = backend.openHost(at(1));
        (void)left->connect(at(2));

        EXPECT_EQ(right->peers().size(), 1U);
    }

    EXPECT_TRUE(right->peers().empty());
}

TEST(LoopbackScheduleTest, Pump_DeliversAPacketAfterTheSenderLeaves)
{
    NiceMock<MockLogger> logger;
    LoopbackBackend backend(
        logger, DeliverySchedule{.delayPumps = 1, .dropped = {}});
    const auto right = backend.openHost(at(2));

    {
        const auto left = backend.openHost(at(1));
        const PeerId peer = left->connect(at(2));
        left->send(peer, bytes("parting"));

        left->pump();
    }

    right->pump();
    right->pump();

    EXPECT_EQ(right->receive().size(), 1U);
    EXPECT_TRUE(right->peers().empty());
}

TEST_F(LoopbackBackendTest, Peers_AreReportedInAscendingOrder)
{
    const auto host = backend.openHost(at(1));
    const auto first = backend.openHost(at(2));
    const auto second = backend.openHost(at(3));
    const auto third = backend.openHost(at(4));

    const PeerId a = host->connect(at(2));
    const PeerId b = host->connect(at(3));
    const PeerId c = host->connect(at(4));

    EXPECT_EQ(host->peers(), (std::vector<PeerId>{a, b, c}));

    host->disconnect(b);

    EXPECT_EQ(host->peers(), (std::vector<PeerId>{a, c}));
}
