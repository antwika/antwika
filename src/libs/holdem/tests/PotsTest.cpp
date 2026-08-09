#include <gtest/gtest.h>

#include <vector>

#include <antwika/holdem/Chips.hpp>
#include <antwika/holdem/HandValue.hpp>
#include <antwika/holdem/Payout.hpp>
#include <antwika/holdem/SeatId.hpp>
#include <antwika/holdem/SidePot.hpp>

#include "Pots.hpp"

using antwika::holdem::buildSidePots;
using antwika::holdem::Chips;
using antwika::holdem::distributePots;
using antwika::holdem::HandValue;
using antwika::holdem::makeSeatId;
using antwika::holdem::Payout;
using antwika::holdem::SeatId;
using antwika::holdem::SidePot;

namespace
{
    [[nodiscard]] std::vector<SeatId> seatsFrom(
        std::initializer_list<std::size_t> indices)
    {
        std::vector<SeatId> seats;
        for (const auto index : indices)
        {
            seats.push_back(makeSeatId(index));
        }
        return seats;
    }

    [[nodiscard]] std::vector<HandValue> valuesFrom(
        std::initializer_list<std::uint32_t> raw)
    {
        std::vector<HandValue> values;
        for (const auto value : raw)
        {
            values.push_back(static_cast<HandValue>(value));
        }
        return values;
    }
}

TEST(PotsTest, BuildSidePots_MakesOneLayerWhenEveryoneMatched)
{
    const std::vector<Chips> committed{100, 100, 100};
    const auto pots = buildSidePots(committed, seatsFrom({0, 1, 2}));

    ASSERT_EQ(pots.size(), 1U);
    EXPECT_EQ(pots[0].amount, 300U);
    EXPECT_EQ(pots[0].contenders, seatsFrom({0, 1, 2}));
}

TEST(PotsTest, BuildSidePots_SplitsAtAShortAllInAmount)
{
    const std::vector<Chips> committed{40, 100, 100};
    const auto pots = buildSidePots(committed, seatsFrom({0, 1, 2}));

    ASSERT_EQ(pots.size(), 2U);
    EXPECT_EQ(pots[0].amount, 120U);
    EXPECT_EQ(pots[0].contenders, seatsFrom({0, 1, 2}));
    EXPECT_EQ(pots[1].amount, 120U);
    EXPECT_EQ(pots[1].contenders, seatsFrom({1, 2}));
}

TEST(PotsTest, BuildSidePots_CountsAFoldedPlayersChipsWithoutSeatingThem)
{
    const std::vector<Chips> committed{100, 100, 30};
    const auto pots = buildSidePots(committed, seatsFrom({0, 1}));

    ASSERT_EQ(pots.size(), 1U);
    EXPECT_EQ(pots[0].amount, 230U);
    EXPECT_EQ(pots[0].contenders, seatsFrom({0, 1}));
}

TEST(PotsTest, BuildSidePots_PutsAFoldedOverBetIntoTheTopLayer)
{
    const std::vector<Chips> committed{100, 150};
    const auto pots = buildSidePots(committed, seatsFrom({0}));

    ASSERT_EQ(pots.size(), 1U);
    EXPECT_EQ(pots[0].amount, 250U);
    EXPECT_EQ(pots[0].contenders, seatsFrom({0}));
}

TEST(PotsTest, BuildSidePots_MakesNoLayersWhenNoEligibleSeatPutAnythingIn)
{
    const std::vector<Chips> committed{0, 0};
    EXPECT_TRUE(buildSidePots(committed, seatsFrom({0, 1})).empty());
}

TEST(PotsTest, DistributePots_GivesTheWholeLayerToTheBestHand)
{
    const std::vector<SidePot> pots{
        SidePot{.amount = 300, .contenders = seatsFrom({0, 1, 2})},
    };
    const auto values = valuesFrom({10, 50, 20});

    const auto payouts =
        distributePots(pots, values, seatsFrom({0, 1, 2}));

    const std::vector<Payout> expected{
        Payout{.seat = makeSeatId(1), .amount = 300},
    };
    EXPECT_EQ(payouts, expected);
}

TEST(PotsTest, DistributePots_SplitsALayerBetweenTiedHands)
{
    const std::vector<SidePot> pots{
        SidePot{.amount = 300, .contenders = seatsFrom({0, 1, 2})},
    };
    const auto values = valuesFrom({50, 50, 20});

    const auto payouts =
        distributePots(pots, values, seatsFrom({0, 1, 2}));

    const std::vector<Payout> expected{
        Payout{.seat = makeSeatId(0), .amount = 150},
        Payout{.seat = makeSeatId(1), .amount = 150},
    };
    EXPECT_EQ(payouts, expected);
}

TEST(PotsTest, DistributePots_GivesAnOddChipToTheFirstSeatLeftOfTheButton)
{
    const std::vector<SidePot> pots{
        SidePot{.amount = 101, .contenders = seatsFrom({0, 1})},
    };
    const auto values = valuesFrom({50, 50});

    const auto payouts = distributePots(pots, values, seatsFrom({1, 0}));

    const std::vector<Payout> expected{
        Payout{.seat = makeSeatId(0), .amount = 50},
        Payout{.seat = makeSeatId(1), .amount = 51},
    };
    EXPECT_EQ(payouts, expected);
}

TEST(PotsTest, DistributePots_AwardsEachLayerToItsOwnBestContender)
{
    const std::vector<SidePot> pots{
        SidePot{.amount = 120, .contenders = seatsFrom({0, 1, 2})},
        SidePot{.amount = 120, .contenders = seatsFrom({1, 2})},
    };
    const auto values = valuesFrom({90, 50, 70});

    const auto payouts =
        distributePots(pots, values, seatsFrom({0, 1, 2}));

    const std::vector<Payout> expected{
        Payout{.seat = makeSeatId(0), .amount = 120},
        Payout{.seat = makeSeatId(2), .amount = 120},
    };
    EXPECT_EQ(payouts, expected);
}

TEST(PotsTest, DistributePots_SumsWhatOneSeatWinsAcrossLayers)
{
    const std::vector<SidePot> pots{
        SidePot{.amount = 60, .contenders = seatsFrom({0, 1})},
        SidePot{.amount = 40, .contenders = seatsFrom({1})},
    };
    const auto values = valuesFrom({10, 20});

    const auto payouts = distributePots(pots, values, seatsFrom({0, 1}));

    const std::vector<Payout> expected{
        Payout{.seat = makeSeatId(1), .amount = 100},
    };
    EXPECT_EQ(payouts, expected);
}

TEST(PotsTest, DistributePots_PaysNobodyWhenThereAreNoLayers)
{
    const std::vector<SidePot> pots;
    const auto values = valuesFrom({10, 20});

    EXPECT_TRUE(distributePots(pots, values, seatsFrom({0, 1})).empty());
}
