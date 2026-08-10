#include <gtest/gtest.h>

#include <antwika/enums/Enumeration.hpp>
#include <antwika/tilemap/TerrainClass.hpp>

using antwika::tilemap::TerrainClass;
using antwika::tilemap::toString;

TEST(TerrainClassTest, TerrainClass_CountsSixClasses)
{
    EXPECT_EQ(antwika::enums::kCount<TerrainClass>, 6U);
}

TEST(TerrainClassTest, ToString_NamesEveryTerrain)
{
    EXPECT_EQ(toString(TerrainClass::Floor), "floor");
    EXPECT_EQ(toString(TerrainClass::Wall), "wall");
    EXPECT_EQ(toString(TerrainClass::Water), "water");
    EXPECT_EQ(toString(TerrainClass::Cliff), "cliff");
    EXPECT_EQ(toString(TerrainClass::Path), "path");
    EXPECT_EQ(toString(TerrainClass::Stair), "stair");
}

TEST(TerrainClassTest, ToString_FallsBackForAValueThatNamesNoTerrain)
{
    EXPECT_EQ(toString(static_cast<TerrainClass>(42)), "unknown");
}
