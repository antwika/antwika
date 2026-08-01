#pragma once

#include <cstddef>
#include <memory>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

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

    /** @brief How many pumps a test waits before giving up on a link. */
    inline constexpr int kPatience = 64;

    /**
     * @brief Every promise INetworkBackend makes, as one test suite.
     *
     * Backends under backends/ cannot be held to the coverage gate,
     * since CI has no network to reach. This suite is what replaces
     * that: a backend is finished when it passes this unmodified.
     *
     * **A backend that cannot do something skips rather than fails.**
     * NetworkCapabilities is how it says so, and the alternative -- a
     * backend pretending to connect so that a test goes green -- is
     * exactly the dishonesty a conformance suite exists to prevent.
     *
     * Nothing here assumes delivery is immediate.
     * Every wait is a bounded run of pumps, so a transport that takes a
     * few of them to settle passes the same tests an in-process one
     * does.
     */
    template <typename Traits>
    class NetworkBackendConformance : public ::testing::Test
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

        // Both ends, because a send leaves on the *sender's* pump.
        // Pumping only the far end waits for what was never sent.
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

    TYPED_TEST_SUITE_P(NetworkBackendConformance);

    TYPED_TEST_P(NetworkBackendConformance, Name_IsNotEmpty)
    {
        EXPECT_FALSE(this->backend->name().empty());
    }

    TYPED_TEST_P(NetworkBackendConformance, Name_DoesNotChange)
    {
        EXPECT_EQ(this->backend->name(), this->backend->name());
    }

    TYPED_TEST_P(NetworkBackendConformance, Capabilities_DoNotChange)
    {
        EXPECT_EQ(
            this->backend->capabilities(), this->backend->capabilities());
    }

    // What a host reports is where it is, not what was asked for.
    // A backend handed port 0 is being asked to pick one.
    TYPED_TEST_P(NetworkBackendConformance, OpenHost_ReturnsAHost)
    {
        const auto host = this->backend->openHost(this->at(0));

        ASSERT_NE(host, nullptr);
        EXPECT_EQ(host->endpoint().host, this->at(0).host);
        EXPECT_EQ(host->endpoint(), host->endpoint());
    }

    TYPED_TEST_P(
        NetworkBackendConformance, OpenHost_RefusesAnEmptyHostName)
    {
        EXPECT_THROW(
            (void)this->backend->openHost(
                Endpoint{.host = "", .port = Port{1}}),
            NetworkError);
    }

    TYPED_TEST_P(NetworkBackendConformance, ANewHostHoldsNoPeers)
    {
        const auto host = this->backend->openHost(this->at(0));

        EXPECT_TRUE(host->peers().empty());
        EXPECT_TRUE(host->receive().empty());
    }

    // A host nobody has talked to hands nothing over, ever.
    // However often it is asked -- see IHost::pump().
    TYPED_TEST_P(NetworkBackendConformance, Pump_OnAQuietHostChangesNothing)
    {
        const auto host = this->backend->openHost(this->at(0));

        host->pump();
        host->pump();

        EXPECT_TRUE(host->peers().empty());
        EXPECT_TRUE(host->receive().empty());
    }

    // Ordinary life rather than a refusal -- see IHost::send().
    TYPED_TEST_P(NetworkBackendConformance, SendingIntoTheVoidIsNoError)
    {
        const auto host = this->backend->openHost(this->at(0));

        host->send(PeerId{1}, this->payload(4));
        host->broadcast(this->payload(4));
        host->disconnect(PeerId{1});

        EXPECT_TRUE(host->peers().empty());
    }

    // A backend that says it does not connect must not pretend to.
    TYPED_TEST_P(NetworkBackendConformance, Connect_MatchesWhatItClaims)
    {
        const auto host = this->backend->openHost(this->at(0));

        if (this->backend->capabilities().connects)
        {
            GTEST_SKIP() << "this backend connects";
        }

        EXPECT_THROW((void)host->connect(this->at(1)), NetworkError);
    }

    // Nobody there is not an error -- see IHost::connect().
    // A non-blocking connect cannot know until pumps later.
    // The answer arrives as the peer never turning up.
    TYPED_TEST_P(NetworkBackendConformance, Connect_ToNobodyNeverLinks)
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

    TYPED_TEST_P(NetworkBackendConformance, Connect_IsSeenByBothHosts)
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

    TYPED_TEST_P(NetworkBackendConformance, Send_ArrivesWhereItWasAimed)
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

    // Handed over once and forgotten -- see IHost::receive().
    TYPED_TEST_P(NetworkBackendConformance, Receive_YieldsAPacketOnce)
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

    TYPED_TEST_P(NetworkBackendConformance, Broadcast_ReachesEveryPeer)
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

    TYPED_TEST_P(NetworkBackendConformance, Send_RefusesAnOversizedPayload)
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

    TYPED_TEST_P(NetworkBackendConformance, Disconnect_IsSeenByBothHosts)
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

    // Nothing is left holding a link to a host that has gone.
    TYPED_TEST_P(NetworkBackendConformance, ClosingAHostFreesItsPeers)
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

    REGISTER_TYPED_TEST_SUITE_P(
        NetworkBackendConformance,
        Name_IsNotEmpty,
        Name_DoesNotChange,
        Capabilities_DoNotChange,
        OpenHost_ReturnsAHost,
        OpenHost_RefusesAnEmptyHostName,
        ANewHostHoldsNoPeers,
        Pump_OnAQuietHostChangesNothing,
        SendingIntoTheVoidIsNoError,
        Connect_MatchesWhatItClaims,
        Connect_ToNobodyNeverLinks,
        Connect_IsSeenByBothHosts,
        Send_ArrivesWhereItWasAimed,
        Receive_YieldsAPacketOnce,
        Broadcast_ReachesEveryPeer,
        Send_RefusesAnOversizedPayload,
        Disconnect_IsSeenByBothHosts,
        ClosingAHostFreesItsPeers);

} // namespace antwika::network::conformance
