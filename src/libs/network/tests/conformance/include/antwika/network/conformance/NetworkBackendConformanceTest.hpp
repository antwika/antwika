#pragma once

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

#include <antwika/network/NetworkCapabilities.hpp>

#include <antwika/log/ILogger.hpp>
#include <antwika/log/mocks/MockLogger.hpp>
#include <antwika/network/Endpoint.hpp>
#include <antwika/network/IHost.hpp>
#include <antwika/network/INetworkBackend.hpp>
#include <antwika/network/NetworkError.hpp>
#include <antwika/network/Packet.hpp>
#include <antwika/network/PeerId.hpp>
#include <antwika/network/Port.hpp>

namespace antwika::network::conformance
{

    using antwika::log::ILogger;
    using antwika::log::mocks::MockLogger;
    using ::testing::NiceMock;

    inline constexpr int kPatience = 64;

    template <typename Traits>
    class NetworkBackendConformanceTest : public ::testing::Test
    {
    protected:
        NiceMock<MockLogger> logger;
        std::unique_ptr<INetworkBackend> backend = Traits::create(logger);

        [[nodiscard]] Endpoint at(unsigned index) const
        {
            return Traits::endpointFor(index);
        }

        [[nodiscard]] Endpoint unreachable() const
        {
            return Traits::nowhere();
        }

        [[nodiscard]] bool linksUp() const
        {
            const auto what = backend->capabilities();

            return what.connects && what.listens;
        }

        static void settle(IHost &left, IHost &right)
        {
            for (int pump = 0; pump < kPatience; ++pump)
            {
                left.pump();
                right.pump();
            }
        }

        [[nodiscard]] static std::vector<Packet> awaited(
            IHost &from, IHost &to)
        {
            for (int pump = 0; pump < kPatience; ++pump)
            {
                from.pump();
                to.pump();

                auto arrived = to.receive();

                if (!arrived.empty())
                {
                    return arrived;
                }
            }

            return {};
        }

        [[nodiscard]] static std::vector<std::byte> payload(std::size_t n)
        {
            std::vector<std::byte> out;

            for (std::size_t index = 0; index < n; ++index)
            {
                out.push_back(static_cast<std::byte>(index & 0xFFU));
            }

            return out;
        }
    };

    TYPED_TEST_SUITE_P(NetworkBackendConformanceTest);

    TYPED_TEST_P(NetworkBackendConformanceTest, Name_IsNotEmpty)
    {
        EXPECT_FALSE(this->backend->name().empty());
    }

    TYPED_TEST_P(NetworkBackendConformanceTest, Name_DoesNotChange)
    {
        const std::string before(this->backend->name());

        const auto host = this->backend->openHost(this->at(0));
        host->pump();

        EXPECT_FALSE(before.empty());
        EXPECT_EQ(this->backend->name(), before);
    }

    TYPED_TEST_P(NetworkBackendConformanceTest, Capabilities_DoNotChange)
    {
        const NetworkCapabilities before = this->backend->capabilities();

        const auto host = this->backend->openHost(this->at(0));
        host->pump();

        EXPECT_EQ(this->backend->capabilities(), before);
    }

    TYPED_TEST_P(NetworkBackendConformanceTest, OpenHost_ReturnsAHost)
    {
        const auto host = this->backend->openHost(this->at(0));

        ASSERT_NE(host, nullptr);
        EXPECT_EQ(host->endpoint().host, this->at(0).host);
    }

    TYPED_TEST_P(NetworkBackendConformanceTest, Endpoint_DoesNotDriftOnceBound)
    {
        const auto host = this->backend->openHost(this->at(0));
        const Endpoint bound = host->endpoint();

        host->pump();
        host->pump();

        EXPECT_FALSE(bound.host.empty());
        EXPECT_EQ(host->endpoint(), bound);
    }

    TYPED_TEST_P(
        NetworkBackendConformanceTest, OpenHost_RefusesAnEmptyHostName)
    {
        EXPECT_THROW(
            (void)this->backend->openHost(
                Endpoint{.host = "", .port = Port{1}}),
            NetworkError);
    }

    TYPED_TEST_P(NetworkBackendConformanceTest, OpenHost_HoldsNoPeers)
    {
        const auto host = this->backend->openHost(this->at(0));

        EXPECT_TRUE(host->peers().empty());
        EXPECT_TRUE(host->receive().empty());
    }

    TYPED_TEST_P(NetworkBackendConformanceTest, Pump_OnAQuietHostChangesNothing)
    {
        const auto host = this->backend->openHost(this->at(0));

        host->pump();
        host->pump();

        EXPECT_TRUE(host->peers().empty());
        EXPECT_TRUE(host->receive().empty());
    }

    TYPED_TEST_P(NetworkBackendConformanceTest, Send_IntoTheVoidIsNoError)
    {
        const auto host = this->backend->openHost(this->at(0));

        host->send(PeerId{1}, this->payload(4));
        host->broadcast(this->payload(4));
        host->disconnect(PeerId{1});

        EXPECT_TRUE(host->peers().empty());
    }

    TYPED_TEST_P(NetworkBackendConformanceTest, Connect_MatchesWhatItClaims)
    {
        const auto host = this->backend->openHost(this->at(0));

        if (this->backend->capabilities().connects)
        {
            GTEST_SKIP() << "this backend connects";
        }

        EXPECT_THROW((void)host->connect(this->at(1)), NetworkError);
    }

    TYPED_TEST_P(NetworkBackendConformanceTest, Connect_ToNobodyNeverLinks)
    {
        if (!this->backend->capabilities().connects)
        {
            GTEST_SKIP() << "this backend does not connect";
        }

        const auto host = this->backend->openHost(this->at(0));

        (void)host->connect(this->unreachable());

        for (int pump = 0; pump < kPatience; ++pump)
        {
            host->pump();
        }

        EXPECT_TRUE(host->peers().empty());
    }

    TYPED_TEST_P(NetworkBackendConformanceTest, Connect_IsSeenByBothHosts)
    {
        if (!this->linksUp())
        {
            GTEST_SKIP() << "this backend does not link two hosts";
        }

        const auto left = this->backend->openHost(this->at(0));
        const auto right = this->backend->openHost(this->at(1));

        const PeerId peer = left->connect(right->endpoint());
        this->settle(*left, *right);

        EXPECT_EQ(left->peers(), std::vector<PeerId>{peer});
        EXPECT_EQ(right->peers().size(), 1U);
    }

    TYPED_TEST_P(NetworkBackendConformanceTest, Send_ArrivesWhereItWasAimed)
    {
        if (!this->linksUp())
        {
            GTEST_SKIP() << "this backend does not link two hosts";
        }

        const auto left = this->backend->openHost(this->at(0));
        const auto right = this->backend->openHost(this->at(1));
        const PeerId peer = left->connect(right->endpoint());
        this->settle(*left, *right);

        left->send(peer, this->payload(8));

        const auto arrived = this->awaited(*left, *right);

        ASSERT_EQ(arrived.size(), 1U);
        EXPECT_EQ(arrived.front().payload, this->payload(8));
        EXPECT_EQ(arrived.front().from, right->peers().front());
    }

    TYPED_TEST_P(NetworkBackendConformanceTest, Receive_YieldsAPacketOnce)
    {
        if (!this->linksUp())
        {
            GTEST_SKIP() << "this backend does not link two hosts";
        }

        const auto left = this->backend->openHost(this->at(0));
        const auto right = this->backend->openHost(this->at(1));
        const PeerId peer = left->connect(right->endpoint());
        this->settle(*left, *right);

        left->send(peer, this->payload(8));

        ASSERT_EQ(this->awaited(*left, *right).size(), 1U);
        EXPECT_TRUE(right->receive().empty());
    }

    TYPED_TEST_P(NetworkBackendConformanceTest, Broadcast_ReachesEveryPeer)
    {
        if (!this->linksUp())
        {
            GTEST_SKIP() << "this backend does not link two hosts";
        }

        const auto left = this->backend->openHost(this->at(0));
        const auto first = this->backend->openHost(this->at(1));
        const auto second = this->backend->openHost(this->at(2));
        (void)left->connect(first->endpoint());
        (void)left->connect(second->endpoint());
        this->settle(*left, *first);
        this->settle(*left, *second);

        left->broadcast(this->payload(4));

        EXPECT_EQ(this->awaited(*left, *first).size(), 1U);
        EXPECT_EQ(this->awaited(*left, *second).size(), 1U);
    }

    TYPED_TEST_P(NetworkBackendConformanceTest, Send_RefusesAnOversizedPayload)
    {
        if (!this->linksUp())
        {
            GTEST_SKIP() << "this backend does not link two hosts";
        }

        const auto left = this->backend->openHost(this->at(0));
        const auto right = this->backend->openHost(this->at(1));
        const PeerId peer = left->connect(right->endpoint());
        const auto huge =
            this->payload(this->backend->capabilities().maxPayloadBytes + 1);

        EXPECT_THROW(left->send(peer, huge), NetworkError);
    }

    TYPED_TEST_P(NetworkBackendConformanceTest, Disconnect_IsSeenByBothHosts)
    {
        if (!this->linksUp())
        {
            GTEST_SKIP() << "this backend does not link two hosts";
        }

        const auto left = this->backend->openHost(this->at(0));
        const auto right = this->backend->openHost(this->at(1));
        const PeerId peer = left->connect(right->endpoint());
        this->settle(*left, *right);

        left->disconnect(peer);
        this->settle(*left, *right);

        EXPECT_TRUE(left->peers().empty());
        EXPECT_TRUE(right->peers().empty());
    }

    TYPED_TEST_P(NetworkBackendConformanceTest, Destructor_FreesAHostsPeers)
    {
        if (!this->linksUp())
        {
            GTEST_SKIP() << "this backend does not link two hosts";
        }

        const auto right = this->backend->openHost(this->at(1));

        {
            const auto left = this->backend->openHost(this->at(0));
            (void)left->connect(right->endpoint());
            this->settle(*left, *right);
        }

        for (int pump = 0; pump < kPatience; ++pump)
        {
            right->pump();
        }

        EXPECT_TRUE(right->peers().empty());
    }

    TYPED_TEST_P(
        NetworkBackendConformanceTest, Send_ThenDisconnectStillArrives)
    {
        if (!this->linksUp())
        {
            GTEST_SKIP() << "this backend does not link two hosts";
        }

        const auto left = this->backend->openHost(this->at(0));
        const auto right = this->backend->openHost(this->at(1));
        const PeerId peer = left->connect(right->endpoint());
        this->settle(*left, *right);

        left->send(peer, this->payload(8));
        left->pump();
        left->disconnect(peer);

        std::vector<Packet> arrived;

        for (int pump = 0; pump < kPatience; ++pump)
        {
            left->pump();
            right->pump();

            auto some = right->receive();

            arrived.insert(arrived.end(), some.begin(), some.end());
        }

        ASSERT_EQ(arrived.size(), 1U);
        EXPECT_EQ(arrived.front().payload, this->payload(8));
        EXPECT_TRUE(right->peers().empty());
    }

    REGISTER_TYPED_TEST_SUITE_P(
        NetworkBackendConformanceTest,
        Name_IsNotEmpty,
        Name_DoesNotChange,
        Capabilities_DoNotChange,
        OpenHost_ReturnsAHost,
        Endpoint_DoesNotDriftOnceBound,
        OpenHost_RefusesAnEmptyHostName,
        OpenHost_HoldsNoPeers,
        Pump_OnAQuietHostChangesNothing,
        Send_IntoTheVoidIsNoError,
        Connect_MatchesWhatItClaims,
        Connect_ToNobodyNeverLinks,
        Connect_IsSeenByBothHosts,
        Send_ArrivesWhereItWasAimed,
        Receive_YieldsAPacketOnce,
        Broadcast_ReachesEveryPeer,
        Send_RefusesAnOversizedPayload,
        Disconnect_IsSeenByBothHosts,
        Send_ThenDisconnectStillArrives,
        Destructor_FreesAHostsPeers);

}
