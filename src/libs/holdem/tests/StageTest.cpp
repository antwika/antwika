#include <gtest/gtest.h>

#include <antwika/holdem/Stage.hpp>

using antwika::holdem::Stage;
using antwika::holdem::toString;

TEST(StageTest, ToString_NamesEveryStage)
{
    EXPECT_EQ(toString(Stage::PreFlop), "pre-flop");
    EXPECT_EQ(toString(Stage::Flop), "flop");
    EXPECT_EQ(toString(Stage::Turn), "turn");
    EXPECT_EQ(toString(Stage::River), "river");
    EXPECT_EQ(toString(Stage::Showdown), "showdown");
}

TEST(StageTest, ToString_FallsBackForAValueThatNamesNoStage)
{
    EXPECT_EQ(toString(static_cast<Stage>(42)), "unknown");
}

TEST(StageTest, Stage_IncreasesFromPreFlopThroughShowdown)
{
    EXPECT_LT(Stage::PreFlop, Stage::Flop);
    EXPECT_LT(Stage::Flop, Stage::Turn);
    EXPECT_LT(Stage::Turn, Stage::River);
    EXPECT_LT(Stage::River, Stage::Showdown);
}
