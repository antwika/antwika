#include "antwika/network/NetworkCapabilities.hpp"

#include <gtest/gtest.h>

using antwika::network::NetworkCapabilities;

namespace
{
    constexpr NetworkCapabilities kReal{
        .connects = true,
        .listens = true,
        .maxPeers = 8,
        .maxPayloadBytes = 1024};
} // namespace

TEST(NetworkCapabilitiesTest, ABackendThatSaysNothingReachesNobody)
{
    constexpr NetworkCapabilities quiet;

    EXPECT_FALSE(quiet.connects);
    EXPECT_FALSE(quiet.listens);
    EXPECT_EQ(quiet.maxPeers, 0U);
    EXPECT_EQ(quiet.maxPayloadBytes, 0U);
}

TEST(NetworkCapabilitiesTest, EqualityComparesEveryFieldIndependently)
{
    EXPECT_EQ(kReal, kReal);

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

// Dialling out and being dialled into are separate abilities.
TEST(NetworkCapabilitiesTest, ConnectingAndListeningAreSaidSeparately)
{
    constexpr NetworkCapabilities dialsOut{
        .connects = true,
        .listens = false,
        .maxPeers = 1,
        .maxPayloadBytes = 64};

    EXPECT_TRUE(dialsOut.connects);
    EXPECT_FALSE(dialsOut.listens);
}
