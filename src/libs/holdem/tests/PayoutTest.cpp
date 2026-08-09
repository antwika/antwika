#include <gtest/gtest.h>

#include <antwika/holdem/Payout.hpp>
#include <antwika/holdem/SeatId.hpp>

using antwika::holdem::makeSeatId;
using antwika::holdem::Payout;

TEST(PayoutTest, OperatorEquals_ComparesEveryFieldIndependently)
{
    const Payout paid{.seat = makeSeatId(2), .amount = 120};

    EXPECT_EQ(paid, (Payout{.seat = makeSeatId(2), .amount = 120}));
    EXPECT_NE(paid, (Payout{.seat = makeSeatId(3), .amount = 120}));
    EXPECT_NE(paid, (Payout{.seat = makeSeatId(2), .amount = 121}));
}
