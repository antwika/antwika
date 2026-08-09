#include <gtest/gtest.h>

#include <cstdint>

#include "antwika/network/PeerId.hpp"

using antwika::network::PeerId;
using antwika::network::rawValue;

TEST(PeerIdTest, RawValue_ReportsTheNumberBehindIt)
{
    constexpr PeerId peer{7};

    EXPECT_EQ(rawValue(peer), 7U);
}

TEST(PeerIdTest, OperatorCompare_ComparesIdsByTheirNumbers)
{
    constexpr PeerId first{1};
    constexpr PeerId second{2};

    EXPECT_EQ(first, PeerId{1});
    EXPECT_NE(first, second);
}

TEST(PeerIdTest, RawValue_CarriesTheWholeWidthOfItsType)
{
    constexpr PeerId last{UINT32_MAX};

    EXPECT_EQ(rawValue(last), UINT32_MAX);
}
