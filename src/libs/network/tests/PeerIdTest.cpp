#include "antwika/network/PeerId.hpp"

#include <gtest/gtest.h>

#include <cstdint>

using antwika::network::PeerId;
using antwika::network::rawValue;

TEST(PeerIdTest, RawValue_ReportsTheNumberBehindIt)
{
    constexpr PeerId peer{7};

    EXPECT_EQ(rawValue(peer), 7U);
}

// Distinct ids compare distinct, which is the whole of what an id is.
TEST(PeerIdTest, TwoIdsCompareByTheirNumbers)
{
    constexpr PeerId first{1};
    constexpr PeerId second{2};

    EXPECT_EQ(first, PeerId{1});
    EXPECT_NE(first, second);
}

// Never reused, so an id has to reach the whole of its width.
TEST(PeerIdTest, RawValue_CarriesTheWholeWidthOfItsType)
{
    constexpr PeerId last{UINT32_MAX};

    EXPECT_EQ(rawValue(last), UINT32_MAX);
}
