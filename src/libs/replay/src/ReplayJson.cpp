#include "antwika/replay/ReplayJson.hpp"

#include <cstdint>
#include <format>
#include <string>

#include <nlohmann/json-schema.hpp>

#include <antwika/replay/EventJson.hpp>
#include <antwika/replay/JsonShapes.hpp>
#include <antwika/replay/ReplayFormatError.hpp>
#include <antwika/replay/SchemaVersion.hpp>
#include <antwika/replay/VersionedDocument.hpp>

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
            shape["properties"]["width"] = countShape();
            shape["properties"]["height"] = countShape();
            return shape;
        }

        nlohmann::json replaySchema()
        {
            nlohmann::json schema;
            schema["$schema"] = "http://json-schema.org/draft-07/schema#";
            schema["title"] = "antwika replay document";
            schema["type"] = "object";
            schema["additionalProperties"] = false;

            schema["$id"] = std::format(
                "https://antwika.dev/schemas/replay-document/{}",
                kReplayDocumentVersion);

            // "canvas" is described but never required.
            // The version stays at 1 for the same reason.
            // Every recording written before the field has neither.
            // Refusing those is what this field must not do.
            //
            // "version" is required, and an older file still loads.
            // replayFromJson migrates before it validates.
            // Migrating stamps the version it arrived at.
            // So this schema only ever sees the current version.
            // That is the point: one schema exists, not one per bump.
            schema["required"] =
                {"magic", "version", "events"}; // GCOVR_EXCL_LINE
            schema["properties"]["magic"]["const"] =
                std::string(detail::kReplayMagic);
            schema["properties"]["version"]["const"] =
                kReplayDocumentVersion;
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

    ReplayDocument replayFromJson(
        const nlohmann::json &j, const MigrationChain &migrations)
    {
        const auto migrated = readVersionedDocument<ReplayFormatError>(
            j,
            migrations,
            replayValidator(),
            "antwika::replay: replay JSON failed schema validation: ");

        ReplayDocument document;
        document.events =
            migrated.at("events").get<std::vector<TickEvent>>();

        const auto canvas = migrated.find("canvas");
        if (canvas != migrated.end())
        {
            document.canvas = gfx::Size{
                .width = canvas->at("width").get<std::uint32_t>(),
                .height = canvas->at("height").get<std::uint32_t>(),
            };
        }
        return document;

        // gcov puts this function's cleanup block on its closing brace.
        // Returning an aggregate that owns a vector is what creates one.
        // No input reaches it: the function is covered, the brace is not.
        // replayToJson below returns by value and reports the same.
    } // GCOVR_EXCL_LINE

    nlohmann::json replayToJson(
        const std::vector<TickEvent> &events,
        std::optional<gfx::Size> canvas)
    {
        nlohmann::json encoded;
        encoded["magic"] = std::string(detail::kReplayMagic);
        encoded["version"] = kReplayDocumentVersion;
        encoded["events"] = events;
        if (canvas.has_value())
        {
            encoded["canvas"]["width"] = canvas->width;
            encoded["canvas"]["height"] = canvas->height;
        }
        return encoded;
    } // GCOVR_EXCL_LINE

} // namespace antwika::replay
