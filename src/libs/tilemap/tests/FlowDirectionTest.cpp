#include <gtest/gtest.h>

#include <antwika/enums/Enumeration.hpp>
#include <antwika/tilemap/FlowDirection.hpp>

using antwika::tilemap::FlowDirection;
using antwika::tilemap::toString;

TEST(FlowDirectionTest, FlowDirection_CountsFourDirections)
{
    EXPECT_EQ(antwika::enums::kCount<FlowDirection>, 4U);
}

TEST(FlowDirectionTest, ToString_NamesEveryDirection)
{
    EXPECT_EQ(toString(FlowDirection::North), "north");
    EXPECT_EQ(toString(FlowDirection::East), "east");
    EXPECT_EQ(toString(FlowDirection::South), "south");
    EXPECT_EQ(toString(FlowDirection::West), "west");
}

TEST(FlowDirectionTest, ToString_FallsBackForAValueThatNamesNoDirection)
{
    EXPECT_EQ(toString(static_cast<FlowDirection>(42)), "unknown");
}
