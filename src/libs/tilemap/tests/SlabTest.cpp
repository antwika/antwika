#include <gtest/gtest.h>

#include <cstdint>
#include <limits>

#include <antwika/tilemap/Overlay.hpp>
#include <antwika/tilemap/Slab.hpp>
#include <antwika/tilemap/TerrainClass.hpp>
#include <antwika/tilemap/WaterAttributes.hpp>

using antwika::tilemap::Overlay;
using antwika::tilemap::Slab;
using antwika::tilemap::TerrainClass;
using antwika::tilemap::WaterAttributes;

TEST(SlabTest, Slab_DefaultsToALitFloorAtLevelZero)
{
    const Slab slab{};

    EXPECT_EQ(slab.level, 0);
    EXPECT_EQ(slab.terrain, TerrainClass::Floor);
    EXPECT_EQ(slab.overlay, Overlay::None);
    EXPECT_EQ(slab.water, WaterAttributes{});
    EXPECT_EQ(slab.light, 255);
}

TEST(SlabTest, Level_HoldsTheFullSignedRange)
{
    Slab slab{};

    slab.level = std::numeric_limits<std::int32_t>::max();
    EXPECT_EQ(slab.level, std::numeric_limits<std::int32_t>::max());

    slab.level = std::numeric_limits<std::int32_t>::min();
    EXPECT_EQ(slab.level, std::numeric_limits<std::int32_t>::min());
}

TEST(SlabTest, OperatorEquals_ComparesEveryField)
{
    const Slab base{
        .level = 3,
        .terrain = TerrainClass::Water,
        .overlay = Overlay::Bridge,
        .water = {.deadly = true},
        .light = 7};
    const auto twin = base;

    EXPECT_EQ(base, twin);

    auto other = base;
    other.level = 4;
    EXPECT_NE(base, other);

    other = base;
    other.terrain = TerrainClass::Cliff;
    EXPECT_NE(base, other);

    other = base;
    other.overlay = Overlay::None;
    EXPECT_NE(base, other);

    other = base;
    other.water.deadly = false;
    EXPECT_NE(base, other);

    other = base;
    other.light = 8;
    EXPECT_NE(base, other);
}
