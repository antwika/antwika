#include <gtest/gtest.h>

#include <antwika/tilemap/FlowDirection.hpp>
#include <antwika/tilemap/WaterAttributes.hpp>

using antwika::tilemap::FlowDirection;
using antwika::tilemap::WaterAttributes;

TEST(WaterAttributesTest, WaterAttributes_DefaultToStillHarmlessWater)
{
    const WaterAttributes water{};

    EXPECT_FALSE(water.deadly);
    EXPECT_FALSE(water.swimmable);
    EXPECT_FALSE(water.current.has_value());
}

TEST(WaterAttributesTest, OperatorEquals_ComparesEveryField)
{
    const WaterAttributes base{
        .deadly = true,
        .swimmable = true,
        .current = FlowDirection::North};
    const auto twin = base;

    EXPECT_EQ(base, twin);

    auto other = base;
    other.deadly = false;
    EXPECT_NE(base, other);

    other = base;
    other.swimmable = false;
    EXPECT_NE(base, other);

    other = base;
    other.current = FlowDirection::South;
    EXPECT_NE(base, other);
}
