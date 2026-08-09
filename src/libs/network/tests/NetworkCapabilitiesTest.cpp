#include <gtest/gtest.h>

#include "antwika/network/NetworkCapabilities.hpp"

using antwika::network::NetworkCapabilities;

namespace
{
    constexpr NetworkCapabilities kReal{
        .connects = true,
        .listens = true,
        .maxPeers = 8,
        .maxPayloadBytes = 1024};
}

TEST(NetworkCapabilitiesTest, Capabilities_DefaultToReachingNobody)
{
    constexpr NetworkCapabilities quiet;

    EXPECT_FALSE(quiet.connects);
    EXPECT_FALSE(quiet.listens);
    EXPECT_EQ(quiet.maxPeers, 0U);
    EXPECT_EQ(quiet.maxPayloadBytes, 0U);
}

TEST(NetworkCapabilitiesTest, OperatorEquals_ComparesEveryFieldIndependently)
{
    const auto twin = kReal;
    EXPECT_EQ(kReal, twin);

    EXPECT_NE(
        kReal,
        (NetworkCapabilities{
            .connects = false,
            .listens = true,
            .maxPeers = 8,
            .maxPayloadBytes = 1024}));

    EXPECT_NE(
        kReal,
        (NetworkCapabilities{
            .connects = true,
            .listens = false,
            .maxPeers = 8,
            .maxPayloadBytes = 1024}));

    EXPECT_NE(
        kReal,
        (NetworkCapabilities{
            .connects = true,
            .listens = true,
            .maxPeers = 9,
            .maxPayloadBytes = 1024}));

    EXPECT_NE(
        kReal,
        (NetworkCapabilities{
            .connects = true,
            .listens = true,
            .maxPeers = 8,
            .maxPayloadBytes = 1025}));
}
