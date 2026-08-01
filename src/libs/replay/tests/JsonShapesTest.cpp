#include <gtest/gtest.h>

#include <cstdint>
#include <limits>

#include <nlohmann/json.hpp>

#include "antwika/replay/JsonShapes.hpp"

using antwika::replay::boundedCountShape;
using antwika::replay::coordinateShape;
using antwika::replay::countShape;
using antwika::replay::wordShape;

TEST(JsonShapesTest, CountShape_IsAWholeNumberThatIsNeverNegative)
{
    const auto shape = countShape();

    EXPECT_EQ(shape.at("type"), "integer");
    EXPECT_EQ(shape.at("minimum"), 0);
}

// Deliberately unbounded above, unlike boundedCountShape().
// A tick count and a seed are read as whatever the file says.
TEST(JsonShapesTest, CountShape_NamesNoLargestValue)
{
    EXPECT_FALSE(countShape().contains("maximum"));
}

TEST(JsonShapesTest, BoundedCountShape_KeepsTheCountAndAddsTheBound)
{
    const auto shape = boundedCountShape(7);

    EXPECT_EQ(shape.at("type"), "integer");
    EXPECT_EQ(shape.at("minimum"), 0);
    EXPECT_EQ(shape.at("maximum"), 7);
}

// The bound is what stops nlohmann narrowing a decode in silence.
// So it has to reach what the widest of these formats decodes into.
TEST(JsonShapesTest, BoundedCountShape_CarriesAWholeInt64)
{
    const auto largest = std::numeric_limits<std::int64_t>::max();

    EXPECT_EQ(boundedCountShape(largest).at("maximum"), largest);
}

TEST(JsonShapesTest, CoordinateShape_IsBoundedByWhatAnInt32Holds)
{
    const auto shape = coordinateShape();

    EXPECT_EQ(shape.at("type"), "integer");
    EXPECT_EQ(shape.at("minimum"),
              std::numeric_limits<std::int32_t>::min());
    EXPECT_EQ(shape.at("maximum"),
              std::numeric_limits<std::int32_t>::max());
}

// A string and nothing else.
// Which word is legal is the decode's question, not the schema's.
TEST(JsonShapesTest, WordShape_ConstrainsNothingButTheType)
{
    const auto shape = wordShape();

    EXPECT_EQ(shape.at("type"), "string");
    EXPECT_EQ(shape.size(), 1U);
}
