#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <nlohmann/json-schema.hpp>
#include <nlohmann/json.hpp>

#include <cstddef>
#include <stdexcept>
#include <string>

#include "antwika/schema/DocumentDepth.hpp"
#include "antwika/schema/JsonSchemas.hpp"
#include "antwika/schema/PayloadJson.hpp"

using antwika::schema::getCountSchema;
using antwika::schema::kMaxDocumentDepth;
using antwika::schema::parseAndValidatePayload;

namespace
{
    class ToyPayloadError final : public std::runtime_error
    {
    public:
        using std::runtime_error::runtime_error;
    };

    nlohmann::json getToySchema()
    {
        nlohmann::json schema;
        schema["type"] = "object";
        schema["additionalProperties"] = false;
        schema["required"] = {"count"};
        schema["properties"]["count"] = getCountSchema();
        return schema;
    }

    const nlohmann::json_schema::json_validator &getToyValidator()
    {
        static const nlohmann::json_schema::json_validator validator(
            getToySchema());
        return validator;
    }

    std::string getPastTheBound()
    {
        std::string text = "7";

        for (std::size_t level = 0; level <= kMaxDocumentDepth;
             ++level)
        {
            text = "[" + text + "]";
        }

        return text;
    }
}

TEST(PayloadJsonTest, ParsePayload_ReturnsAnAcceptedPayload)
{
    const auto parsedPayload = parseAndValidatePayload<ToyPayloadError>(
        R"({"count": 4})", getToyValidator(), "toy payload");

    EXPECT_EQ(parsedPayload["count"], 4);
}

TEST(PayloadJsonTest, ParsePayload_RefusesAPayloadThatIsNotJson)
{
    EXPECT_THROW(
        (void)parseAndValidatePayload<ToyPayloadError>(
            "{count", getToyValidator(), "toy payload"),
        ToyPayloadError);
}

TEST(PayloadJsonTest, ParsePayload_RefusesWhatTheSchemaRejects)
{
    EXPECT_THROW(
        (void)parseAndValidatePayload<ToyPayloadError>(
            R"({"count": "four"})", getToyValidator(), "toy payload"),
        ToyPayloadError);
}

TEST(PayloadJsonTest, ParsePayload_RefusesNestingPastTheBound)
{
    EXPECT_THAT(
        []
        {
            (void)parseAndValidatePayload<ToyPayloadError>(
                getPastTheBound(), getToyValidator(), "toy payload");
        },
        testing::ThrowsMessage<ToyPayloadError>(
            testing::HasSubstr("nests deeper")));
}
