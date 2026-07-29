#include "antwika/replay/ReplayJson.hpp"

#include <string>

#include <nlohmann/json-schema.hpp>

#include <antwika/replay/EventJson.hpp>
#include <antwika/replay/ReplayFormatError.hpp>

#include "EventSchema.hpp"
#include "ReplayFormat.hpp"

namespace antwika::replay
{

    namespace
    {
        nlohmann::json replaySchema()
        {
            nlohmann::json schema;
            schema["$schema"] = "http://json-schema.org/draft-07/schema#";
            schema["title"] = "antwika replay document";
            schema["type"] = "object";
            schema["additionalProperties"] = false;
            schema["required"] =
                {"magic", "version", "events"}; // GCOVR_EXCL_LINE
            schema["properties"]["magic"]["const"] =
                std::string(detail::kReplayMagic);
            schema["properties"]["version"]["const"] =
                detail::kReplayFormatVersion;
            schema["properties"]["events"]["type"] = "array";
            schema["properties"]["events"]["items"] =
                detail::tickEventShape();
            return schema;
        }

        const nlohmann::json_schema::json_validator &replayValidator()
        {
            static const nlohmann::json_schema::json_validator validator(
                replaySchema()); // GCOVR_EXCL_LINE
            return validator;
        }
    } // namespace

    std::vector<TickEvent> replayFromJson(const nlohmann::json &j)
    {
        try
        {
            replayValidator().validate(j);
        }
        catch (const std::exception &error) // GCOVR_EXCL_LINE
        {
            throw ReplayFormatError(
                std::string(
                    "antwika::replay: replay JSON failed schema "
                    "validation: ") +
                error.what());
        }
        return j.at("events").get<std::vector<TickEvent>>();
    }

    nlohmann::json replayToJson(const std::vector<TickEvent> &events)
    {
        nlohmann::json document;
        document["magic"] = std::string(detail::kReplayMagic);
        document["version"] = detail::kReplayFormatVersion;
        document["events"] = events;
        return document;
    } // GCOVR_EXCL_LINE

} // namespace antwika::replay
