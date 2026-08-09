#pragma once

#include <nlohmann/json-schema.hpp>

#include <string>
#include <string_view>

#include "antwika/replay/DocumentDepth.hpp"

namespace antwika::replay
{

    template <typename ErrorT>
    [[nodiscard]] nlohmann::json parseAndValidatePayload(
        const std::string &payload,
        const nlohmann::json_schema::json_validator &validator,
        std::string_view context)
    {
        nlohmann::json parsed;
        try
        {
            parsed = nlohmann::json::parse(payload);
        }
        catch (const nlohmann::json::parse_error &) // GCOVR_EXCL_LINE
        {
            throw ErrorT(std::string(context) + " is not valid JSON");
        }

        if (nestsTooDeep(parsed))
        {
            throw ErrorT(
                std::string(context)
                + " nests deeper than any event this project writes");
        }

        try
        {
            validator.validate(parsed);
        }
        catch (const std::exception &error) // GCOVR_EXCL_LINE
        {
            throw ErrorT(
                std::string(context) + " failed schema validation: "
                + error.what());
        }
        return parsed;
    }

}
