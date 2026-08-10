#include <gtest/gtest.h>

#include <antwika/tilemap/MapHeader.hpp>
#include <antwika/tilemap/Rgb.hpp>

using antwika::tilemap::kSchemaVersion;
using antwika::tilemap::MapHeader;
using antwika::tilemap::Rgb;

TEST(MapHeaderTest, MapHeader_DefaultsToSchemaVersionTwo)
{
    const MapHeader header{};

    EXPECT_EQ(header.schemaVersion, 2U);
    EXPECT_EQ(kSchemaVersion, 2U);
}

TEST(MapHeaderTest, MapHeader_DefaultsToBlackInkOnWhitePaper)
{
    const MapHeader header{};

    EXPECT_EQ(header.ink, (Rgb{.red = 0, .green = 0, .blue = 0}));
    EXPECT_EQ(
        header.paper, (Rgb{.red = 255, .green = 255, .blue = 255}));
}

TEST(MapHeaderTest, OperatorEquals_ComparesEveryField)
{
    const MapHeader base{
        .id = "wakewater-01",
        .schemaVersion = 2,
        .ink = {.red = 10, .green = 20, .blue = 30},
        .paper = {.red = 200, .green = 210, .blue = 220}};
    const auto twin = base;

    EXPECT_EQ(base, twin);

    auto other = base;
    other.id = "wakewater-02";
    EXPECT_NE(base, other);

    other = base;
    other.schemaVersion = 3;
    EXPECT_NE(base, other);

    other = base;
    other.ink.red = 11;
    EXPECT_NE(base, other);

    other = base;
    other.paper.blue = 221;
    EXPECT_NE(base, other);
}
