#include "antwika/replay/ReplayJson.hpp"

#include <cstdint>
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
        nlohmann::json canvasShape()
        {
            nlohmann::json shape;
            shape["type"] = "object";
            shape["additionalProperties"] = false;
            shape["required"] = {"width", "height"}; // GCOVR_EXCL_LINE
            shape["properties"]["width"]["type"] = "integer";
            shape["properties"]["width"]["minimum"] = 0;
            shape["properties"]["height"]["type"] = "integer";
            shape["properties"]["height"]["minimum"] = 0;
            return shape;
        }

        nlohmann::json replaySchema()
        {
            nlohmann::json schema;
            schema["$schema"] = "http://json-schema.org/draft-07/schema#";
            schema["title"] = "antwika replay document";
            schema["type"] = "object";
            schema["additionalProperties"] = false;

            // "canvas" is described but never required.
            // The version stays at 1 for the same reason.
            // Every recording written before the field has neither.
            // Refusing those is what this field must not do.
            schema["required"] =
                {"magic", "version", "events"}; // GCOVR_EXCL_LINE
            schema["properties"]["magic"]["const"] =
                std::string(detail::kReplayMagic);
            schema["properties"]["version"]["const"] =
                detail::kReplayFormatVersion;
            schema["properties"]["events"]["type"] = "array";
            schema["properties"]["events"]["items"] =
                detail::tickEventShape();
            schema["properties"]["canvas"] = canvasShape();
            return schema;
        }

        const nlohmann::json_schema::json_validator &replayValidator()
        {
            static const nlohmann::json_schema::json_validator validator(
                replaySchema()); // GCOVR_EXCL_LINE
            return validator;
        }
    } // namespace

    ReplayDocument replayFromJson(const nlohmann::json &j)
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

        ReplayDocument document;
        document.events = j.at("events").get<std::vector<TickEvent>>();

        const auto canvas = j.find("canvas");
        if (canvas != j.end())
        {
            document.canvas = gfx::Size{
                .width = canvas->at("width").get<std::uint32_t>(),
                .height = canvas->at("height").get<std::uint32_t>(),
            };
        }
        return document;
    } // GCOVR_EXCL_LINE

    nlohmann::json replayToJson(
        const std::vector<TickEvent> &events,
        std::optional<gfx::Size> canvas)
    {
        nlohmann::json encoded;
        encoded["magic"] = std::string(detail::kReplayMagic);
        encoded["version"] = detail::kReplayFormatVersion;
        encoded["events"] = events;
        if (canvas.has_value())
        {
            encoded["canvas"]["width"] = canvas->width;
            encoded["canvas"]["height"] = canvas->height;
        }
        return encoded;
    } // GCOVR_EXCL_LINE

} // namespace antwika::replay
