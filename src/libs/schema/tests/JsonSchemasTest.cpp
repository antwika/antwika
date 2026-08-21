#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

#include <cstdint>
#include <limits>

#include "antwika/schema/JsonSchemas.hpp"

using antwika::schema::boundedCountSchema;
using antwika::schema::coordinateSchema;
using antwika::schema::countSchema;
using antwika::schema::documentSchema;
using antwika::schema::objectSchema;
using antwika::schema::requiredSchema;
using antwika::schema::validatorFor;
using antwika::schema::wordSchema;

namespace
{
    nlohmann::json pairSchema()
    {
        nlohmann::json schema = documentSchema("a pair", {"x", "y"});
        schema["properties"]["x"] = countSchema();
        schema["properties"]["y"] = countSchema();
        return schema;
    }
}

TEST(JsonSchemasTest, CountSchema_IsAWholeNumberThatIsNeverNegative)
{
    const auto shape = countSchema();

    EXPECT_EQ(shape.at("type"), "integer");
    EXPECT_EQ(shape.at("minimum"), 0);
}

TEST(JsonSchemasTest, CountSchema_NamesNoLargestValue)
{
    EXPECT_FALSE(countSchema().contains("maximum"));
}

TEST(JsonSchemasTest, BoundedCountSchema_KeepsTheCountAndAddsTheBound)
{
    const auto shape = boundedCountSchema(7);

    EXPECT_EQ(shape.at("type"), "integer");
    EXPECT_EQ(shape.at("minimum"), 0);
    EXPECT_EQ(shape.at("maximum"), 7);
}

TEST(JsonSchemasTest, BoundedCountSchema_CarriesAWholeInt64)
{
    const auto largest = std::numeric_limits<std::int64_t>::max();

    EXPECT_EQ(boundedCountSchema(largest).at("maximum"), largest);
}

TEST(JsonSchemasTest, CoordinateSchema_IsBoundedByWhatAnInt32Holds)
{
    const auto shape = coordinateSchema();

    EXPECT_EQ(shape.at("type"), "integer");
    EXPECT_EQ(shape.at("minimum"),
              std::numeric_limits<std::int32_t>::min());
    EXPECT_EQ(shape.at("maximum"),
              std::numeric_limits<std::int32_t>::max());
}

TEST(JsonSchemasTest, WordSchema_ConstrainsNothingButTheType)
{
    const auto shape = wordSchema();

    EXPECT_EQ(shape.at("type"), "string");
    EXPECT_EQ(shape.size(), 1U);
}

TEST(JsonSchemasTest, RequiredSchema_ListsTheMembersInTheOrderGiven)
{
    const auto shape = requiredSchema({"magic", "cells"});

    ASSERT_TRUE(shape.is_array());
    ASSERT_EQ(shape.size(), 2U);
    EXPECT_EQ(shape.at(0), "magic");
    EXPECT_EQ(shape.at(1), "cells");
}

TEST(JsonSchemasTest, RequiredSchema_ListsNothingForNoMembers)
{
    EXPECT_EQ(requiredSchema({}), nlohmann::json::array());
}

TEST(JsonSchemasTest, ObjectSchema_IsAClosedObjectOverTheMembersNamed)
{
    const auto shape = objectSchema({"x", "y"});

    EXPECT_EQ(shape.at("type"), "object");
    EXPECT_FALSE(shape.at("additionalProperties").get<bool>());
    EXPECT_EQ(shape.at("required"), requiredSchema({"x", "y"}));
}

TEST(JsonSchemasTest, ObjectSchema_NamesNoDialectAndNoTitle)
{
    const auto shape = objectSchema({"x"});

    EXPECT_FALSE(shape.contains("$schema"));
    EXPECT_FALSE(shape.contains("title"));
}

TEST(JsonSchemasTest, DocumentSchema_KeepsTheObjectAndSaysWhatItIs)
{
    const auto shape = documentSchema("antwika something", {"x"});

    EXPECT_EQ(shape.at("type"), "object");
    EXPECT_FALSE(shape.at("additionalProperties").get<bool>());
    EXPECT_EQ(shape.at("required"), requiredSchema({"x"}));
    EXPECT_EQ(shape.at("$schema"),
              "http://json-schema.org/draft-07/schema#");
    EXPECT_EQ(shape.at("title"), "antwika something");
}

TEST(JsonSchemasTest, ValidatorFor_AcceptsADocumentThatFitsTheSchema)
{
    const nlohmann::json document{{"x", 1}, {"y", 2}};

    EXPECT_NO_THROW(validatorFor<pairSchema>().validate(document));
}

TEST(JsonSchemasTest, ValidatorFor_RefusesADocumentThatDoesNot)
{
    const nlohmann::json document{{"x", 1}};

    EXPECT_ANY_THROW(validatorFor<pairSchema>().validate(document));
}

TEST(JsonSchemasTest, ValidatorFor_AnswersWithTheOneValidator)
{
    EXPECT_EQ(&validatorFor<pairSchema>(), &validatorFor<pairSchema>());
}
