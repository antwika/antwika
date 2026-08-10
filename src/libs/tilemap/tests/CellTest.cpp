#include <gtest/gtest.h>

#include <cstdint>
#include <limits>

#include <antwika/tilemap/Cell.hpp>
#include <antwika/tilemap/Overlay.hpp>
#include <antwika/tilemap/TerrainClass.hpp>
#include <antwika/tilemap/WaterAttributes.hpp>

using antwika::tilemap::Cell;
using antwika::tilemap::Overlay;
using antwika::tilemap::TerrainClass;
using antwika::tilemap::WaterAttributes;

TEST(CellTest, Cell_DefaultsToALitFloorAtHeightZero)
{
    const Cell cell{};

    EXPECT_EQ(cell.height, 0);
    EXPECT_EQ(cell.terrain, TerrainClass::Floor);
    EXPECT_EQ(cell.overlay, Overlay::None);
    EXPECT_EQ(cell.water, WaterAttributes{});
    EXPECT_EQ(cell.light, 255);
}

TEST(CellTest, Height_HoldsTheFullSignedRange)
{
    Cell cell{};

    cell.height = std::numeric_limits<std::int32_t>::max();
    EXPECT_EQ(cell.height, std::numeric_limits<std::int32_t>::max());

    cell.height = std::numeric_limits<std::int32_t>::min();
    EXPECT_EQ(cell.height, std::numeric_limits<std::int32_t>::min());
}

TEST(CellTest, OperatorEquals_ComparesEveryField)
{
    const Cell base{
        .height = 3,
        .terrain = TerrainClass::Water,
        .overlay = Overlay::Bridge,
        .water = {.deadly = true},
        .light = 7};
    const auto twin = base;

    EXPECT_EQ(base, twin);

    auto other = base;
    other.height = 4;
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
