#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstddef>
#include <stdexcept>
#include <string>

#include <nlohmann/json-schema.hpp>
#include <nlohmann/json.hpp>

#include "antwika/replay/DocumentDepth.hpp"
#include "antwika/replay/JsonShapes.hpp"
#include "antwika/replay/PayloadJson.hpp"

using antwika::replay::countShape;
using antwika::replay::kMaxDocumentDepth;
using antwika::replay::parseAndValidatePayload;

namespace
{
    // A payload format of its own, with an error type of its own.
    // It stands in for the app sinks that keep one each.
    class ToyPayloadError final : public std::runtime_error
    {
    public:
        using std::runtime_error::runtime_error;
    };

    nlohmann::json toySchema()
    {
        nlohmann::json schema;
        schema["type"] = "object";
        schema["additionalProperties"] = false;
        schema["required"] = {"count"};
        schema["properties"]["count"] = countShape();
        return schema;
    }

    const nlohmann::json_schema::json_validator &toyValidator()
    {
        static const nlohmann::json_schema::json_validator validator(
            toySchema());
        return validator;
    }

    // A number under one more level of arrays than the bound allows.
    std::string pastTheBound()
    {
        std::string text = "7";

        for (std::size_t level = 0; level <= kMaxDocumentDepth;
             ++level)
        {
            text = "[" + text + "]";
        }

        return text;
    }
} // namespace

TEST(PayloadJsonTest, ReturnsAPayloadTheSchemaAccepts)
{
    const auto parsed = parseAndValidatePayload<ToyPayloadError>(
        R"({"count": 4})", toyValidator(), "toy payload");

    EXPECT_EQ(parsed["count"], 4);
}

TEST(PayloadJsonTest, RefusesAPayloadThatIsNotJson)
{
    EXPECT_THROW(
        (void)parseAndValidatePayload<ToyPayloadError>(
            "{count", toyValidator(), "toy payload"),
        ToyPayloadError);
}

TEST(PayloadJsonTest, RefusesAPayloadTheSchemaRejects)
{
    EXPECT_THROW(
        (void)parseAndValidatePayload<ToyPayloadError>(
            R"({"count": "four"})", toyValidator(), "toy payload"),
        ToyPayloadError);
}

// A record is depth two, but its payload is a free string.
// The validator's refusal path serializes what it refuses.
// That recursion is what the guard keeps off the stack.
// The schema would also refuse this payload.
// Asserting the message proves the depth guard got there first.
TEST(PayloadJsonTest, RefusesAPayloadNestedPastTheDepthBound)
{
    EXPECT_THAT(
        []
        {
            (void)parseAndValidatePayload<ToyPayloadError>(
                pastTheBound(), toyValidator(), "toy payload");
        },
        testing::ThrowsMessage<ToyPayloadError>(
            testing::HasSubstr("nests deeper")));
}
