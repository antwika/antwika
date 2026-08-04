#include <gtest/gtest.h>

#include "antwika/poker/RoomSummary.hpp"

using antwika::poker::RoomSummary;

namespace
{
    [[nodiscard]] RoomSummary sample()
    {
        return RoomSummary{
            .handsPlayed = 12,
            .balances = {{"alice", 400}, {"bob", 600}},
            .chipsLeftOnTable = 0,
            .console = {},
        };
    }
} // namespace

TEST(RoomSummaryTest, Equality_ComparesEveryFieldIndependently)
{
    const auto summary = sample();

    EXPECT_EQ(summary, sample());

    auto fewerHands = sample();
    fewerHands.handsPlayed = 11;
    EXPECT_NE(summary, fewerHands);

    auto otherBalances = sample();
    otherBalances.balances["alice"] = 401;
    EXPECT_NE(summary, otherBalances);

    auto chipsStranded = sample();
    chipsStranded.chipsLeftOnTable = 25;
    EXPECT_NE(summary, chipsStranded);
}
