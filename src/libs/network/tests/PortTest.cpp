#include "antwika/network/Port.hpp"

#include <gtest/gtest.h>

#include <cstdint>

using antwika::network::Port;
using antwika::network::rawValue;

TEST(PortTest, RawValue_ReportsTheNumberBehindIt)
{
    constexpr Port port{8080};

    EXPECT_EQ(rawValue(port), 8080U);
}

TEST(PortTest, TwoPortsCompareByTheirNumbers)
{
    constexpr Port first{80};
    constexpr Port second{443};

    EXPECT_EQ(first, Port{80});
    EXPECT_NE(first, second);
}

TEST(PortTest, RawValue_CarriesTheWholeWidthOfItsType)
{
    constexpr Port last{UINT16_MAX};

    EXPECT_EQ(rawValue(last), UINT16_MAX);
}
