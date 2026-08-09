#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

#include <cstdint>
#include <limits>

#include "antwika/replay/JsonShapes.hpp"

using antwika::replay::boundedCountShape;
using antwika::replay::coordinateShape;
using antwika::replay::countShape;
using antwika::replay::documentShape;
using antwika::replay::objectShape;
using antwika::replay::requiredShape;
using antwika::replay::validatorFor;
using antwika::replay::wordShape;

namespace
{
    nlohmann::json pairSchema()
    {
        nlohmann::json schema = documentShape("a pair", {"x", "y"});
        schema["properties"]["x"] = countShape();
        schema["properties"]["y"] = countShape();
        return schema;
    }
}

TEST(JsonShapesTest, CountShape_IsAWholeNumberThatIsNeverNegative)
{
    const auto shape = countShape();

    EXPECT_EQ(shape.at("type"), "integer");
    EXPECT_EQ(shape.at("minimum"), 0);
}

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

TEST(JsonShapesTest, WordShape_ConstrainsNothingButTheType)
{
    const auto shape = wordShape();

    EXPECT_EQ(shape.at("type"), "string");
    EXPECT_EQ(shape.size(), 1U);
}

TEST(JsonShapesTest, RequiredShape_ListsTheMembersInTheOrderGiven)
{
    const auto shape = requiredShape({"magic", "cells"});

    ASSERT_TRUE(shape.is_array());
    ASSERT_EQ(shape.size(), 2U);
    EXPECT_EQ(shape.at(0), "magic");
    EXPECT_EQ(shape.at(1), "cells");
}

TEST(JsonShapesTest, RequiredShape_ListsNothingForNoMembers)
{
    EXPECT_EQ(requiredShape({}), nlohmann::json::array());
}

TEST(JsonShapesTest, ObjectShape_IsAClosedObjectOverTheMembersNamed)
{
    const auto shape = objectShape({"x", "y"});

    EXPECT_EQ(shape.at("type"), "object");
    EXPECT_FALSE(shape.at("additionalProperties").get<bool>());
    EXPECT_EQ(shape.at("required"), requiredShape({"x", "y"}));
}

TEST(JsonShapesTest, ObjectShape_NamesNoDialectAndNoTitle)
{
    const auto shape = objectShape({"x"});

    EXPECT_FALSE(shape.contains("$schema"));
    EXPECT_FALSE(shape.contains("title"));
}

TEST(JsonShapesTest, DocumentShape_KeepsTheObjectAndSaysWhatItIs)
{
    const auto shape = documentShape("antwika something", {"x"});

    EXPECT_EQ(shape.at("type"), "object");
    EXPECT_FALSE(shape.at("additionalProperties").get<bool>());
    EXPECT_EQ(shape.at("required"), requiredShape({"x"}));
    EXPECT_EQ(shape.at("$schema"),
              "http://json-schema.org/draft-07/schema#");
    EXPECT_EQ(shape.at("title"), "antwika something");
}

TEST(JsonShapesTest, ValidatorFor_AcceptsADocumentThatFitsTheSchema)
{
    const nlohmann::json document{{"x", 1}, {"y", 2}};

    EXPECT_NO_THROW(validatorFor<pairSchema>().validate(document));
}

TEST(JsonShapesTest, ValidatorFor_RefusesADocumentThatDoesNot)
{
    const nlohmann::json document{{"x", 1}};

    EXPECT_ANY_THROW(validatorFor<pairSchema>().validate(document));
}

TEST(JsonShapesTest, ValidatorFor_AnswersWithTheOneValidator)
{
    EXPECT_EQ(&validatorFor<pairSchema>(), &validatorFor<pairSchema>());
}
