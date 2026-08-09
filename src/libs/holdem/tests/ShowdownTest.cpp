#include <gtest/gtest.h>

#include <string_view>
#include <vector>

#include <antwika/holdem/Card.hpp>
#include <antwika/holdem/CardText.hpp>
#include <antwika/holdem/HandValue.hpp>
#include <antwika/holdem/Seat.hpp>
#include <antwika/holdem/SeatId.hpp>

#include "Showdown.hpp"

using antwika::holdem::HandValue;
using antwika::holdem::makeSeatId;
using antwika::holdem::parseCards;
using antwika::holdem::scoreShowdown;
using antwika::holdem::scoreWithoutShowdown;
using antwika::holdem::Seat;

namespace
{
    const std::vector<antwika::holdem::Card> kBoard =
        parseCards("2h 7d 9s Jd 3c");

    [[nodiscard]] Seat showing(std::string_view holeCards)
    {
        Seat seat;
        seat.occupied = true;
        seat.inHand = true;
        const auto cards = parseCards(holeCards);
        seat.holeCards[0] = cards[0];
        seat.holeCards[1] = cards[1];

        return seat;
    }

    [[nodiscard]] Seat folded(std::string_view holeCards)
    {
        auto seat = showing(holeCards);
        seat.inHand = false;

        return seat;
    }

    [[nodiscard]] std::vector<Seat> table()
    {
        return {
            showing("Ac Ad"),
            showing("Kc Kd"),
            showing("Ks Kh"),
            folded("4c 4d"),
        };
    }
}

TEST(ShowdownTest, ScoreShowdown_ScoresOneEntryPerSeatStillIn)
{
    const auto seats = table();

    const auto scores = scoreShowdown(seats, kBoard);

    EXPECT_EQ(scores.entries.size(), 3U);
    EXPECT_EQ(scores.values.size(), seats.size());
}

TEST(ShowdownTest, ScoreShowdown_LeavesAFoldedSeatUnscored)
{
    const auto seats = table();

    const auto scores = scoreShowdown(seats, kBoard);

    EXPECT_EQ(scores.values.back(), HandValue{});
    ASSERT_FALSE(scores.entries.empty());
    for (const auto &entry : scores.entries)
    {
        EXPECT_NE(entry.seat, makeSeatId(3));
    }
}

TEST(ShowdownTest, ScoreShowdown_IndexesEveryValueBySeat)
{
    const auto seats = table();

    const auto scores = scoreShowdown(seats, kBoard);

    EXPECT_GT(scores.values[0], scores.values[1]);
    EXPECT_EQ(scores.values[1], scores.values[2]);
}

TEST(ShowdownTest, ScoreShowdown_PutsTheStrongestHandFirst)
{
    const auto seats = table();

    const auto scores = scoreShowdown(seats, kBoard);

    EXPECT_EQ(scores.entries[0].seat, makeSeatId(0));
    EXPECT_EQ(scores.entries[0].holeCards[0], parseCards("Ac").front());
    EXPECT_GT(scores.entries[0].value, scores.entries[1].value);
}

TEST(ShowdownTest, ScoreShowdown_KeepsTiedHandsInSeatOrder)
{
    const auto seats = table();

    const auto scores = scoreShowdown(seats, kBoard);

    ASSERT_EQ(scores.entries.size(), 3U);
    EXPECT_EQ(scores.entries[1].value, scores.entries[2].value);
    EXPECT_EQ(scores.entries[1].seat, makeSeatId(1));
    EXPECT_EQ(scores.entries[2].seat, makeSeatId(2));
}

TEST(ShowdownTest, ScoreWithoutShowdown_ComparesNothingAtAll)
{
    const auto scores = scoreWithoutShowdown(4);

    EXPECT_TRUE(scores.entries.empty());
    EXPECT_EQ(scores.values, std::vector<HandValue>(4, HandValue{}));
}
