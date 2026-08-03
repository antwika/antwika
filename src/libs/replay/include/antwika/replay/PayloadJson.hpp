#pragma once

#include <string>
#include <string_view>

#include <nlohmann/json-schema.hpp>

#include "antwika/replay/DocumentDepth.hpp"

namespace antwika::replay
{

    /**
     * @brief Parse an event payload as JSON and validate it against a
     * schema, translating either failure into a caller-chosen exception.
     *
     * Every app event with a structured payload (`life.toggle_cell`,
     * `game.score_increment`, `task.submit`, ...) repeats the same two
     * steps -- parse the payload string as JSON, then validate its shape
     * -- differing only in which schema and which exception type apply.
     * This is that shared step, parameterized over both.
     * @tparam ErrorT Exception type to throw on failure, constructible
     * from a `std::string` message.
     * @param payload The event's raw payload string.
     * @param validator Schema validator for this event's payload shape.
     * @param context Leading text for the thrown message, e.g.
     * `"BoardSink: life.toggle_cell payload"`.
     * @return The parsed, schema-valid payload.
     * @throws ErrorT If the payload is not valid JSON, or fails
     * validation against schema.
     */
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

        // A record is depth two, but its payload is a free string.
        // The validator's refusal path serializes what it refuses.
        // That recurses per level, so depth is refused first here.
        // See DocumentDepth.hpp.
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

} // namespace antwika::replay
