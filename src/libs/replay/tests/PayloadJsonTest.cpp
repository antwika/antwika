#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <nlohmann/json-schema.hpp>
#include <nlohmann/json.hpp>

#include <cstddef>
#include <stdexcept>
#include <string>

#include "antwika/replay/DocumentDepth.hpp"
#include "antwika/replay/JsonShapes.hpp"
#include "antwika/replay/PayloadJson.hpp"

using antwika::replay::countShape;
using antwika::replay::kMaxDocumentDepth;
using antwika::replay::parseAndValidatePayload;

namespace
{
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
}

TEST(PayloadJsonTest, ParsePayload_ReturnsAnAcceptedPayload)
{
    const auto parsed = parseAndValidatePayload<ToyPayloadError>(
        R"({"count": 4})", toyValidator(), "toy payload");

    EXPECT_EQ(parsed["count"], 4);
}

TEST(PayloadJsonTest, ParsePayload_RefusesAPayloadThatIsNotJson)
{
    EXPECT_THROW(
        (void)parseAndValidatePayload<ToyPayloadError>(
            "{count", toyValidator(), "toy payload"),
        ToyPayloadError);
}

TEST(PayloadJsonTest, ParsePayload_RefusesWhatTheSchemaRejects)
{
    EXPECT_THROW(
        (void)parseAndValidatePayload<ToyPayloadError>(
            R"({"count": "four"})", toyValidator(), "toy payload"),
        ToyPayloadError);
}

TEST(PayloadJsonTest, ParsePayload_RefusesNestingPastTheBound)
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
