#pragma once

#include <nlohmann/json-schema.hpp>

#include <string>
#include <string_view>

#include "antwika/schema/DocumentDepth.hpp"

namespace antwika::schema
{

    template <typename ErrorT>
    [[nodiscard]] nlohmann::json parseAndValidatePayload(
        const std::string &payload,
        const nlohmann::json_schema::json_validator &validator,
        std::string_view context)
    {
        nlohmann::json parsedJson;
        try
        {
            parsedJson = nlohmann::json::parse(payload);
        }
        catch (const nlohmann::json::parse_error &) // GCOVR_EXCL_LINE
        {
            throw ErrorT(std::string(context) + " is not valid JSON");
        }

        if (exceedsMaxDepth(parsedJson))
        {
            throw ErrorT(
                std::string(context)
                + " nests deeper than any event this project writes");
        }

        try
        {
            validator.validate(parsedJson);
        }
        catch (const std::exception &error) // GCOVR_EXCL_LINE
        {
            throw ErrorT(
                std::string(context) + " failed schema validation: "
                + error.what());
        }
        return parsedJson;
    }

}
