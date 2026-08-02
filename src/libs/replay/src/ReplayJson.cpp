#include "antwika/replay/ReplayJson.hpp"

#include <cstddef>
#include <cstdint>
#include <format>
#include <limits>
#include <optional>
#include <string>
#include <utility>

#include <nlohmann/json-schema.hpp>

#include <antwika/replay/EventJson.hpp>
#include <antwika/replay/JsonShapes.hpp>
#include <antwika/replay/ReplayFormatError.hpp>
#include <antwika/replay/SchemaVersion.hpp>
#include <antwika/replay/VersionedDocument.hpp>
#include <antwika/time/Tick.hpp>

#include "EventSchema.hpp"
#include "ReplayFormat.hpp"

namespace antwika::replay
{

    namespace
    {
        nlohmann::json canvasShape()
        {
            // Bounded because the decode below is get<std::uint32_t>().
            // nlohmann takes the low bytes of anything wider in silence.
            // An unbounded shape let "width": 4294967297 validate.
            // And read back as 1.
            // Which is the trap boundedCountShape exists to stop.
            const nlohmann::json extent = boundedCountShape(
                std::numeric_limits<std::uint32_t>::max());

            nlohmann::json shape;
            shape["type"] = "object";
            shape["additionalProperties"] = false;
            shape["required"] = {"width", "height"}; // GCOVR_EXCL_LINE
            shape["properties"]["width"] = extent;
            shape["properties"]["height"] = extent;
            return shape;
        }

        // "version" is described here and never pinned to one value.
        // Every other schema here describes the current version alone.
        // Migrating comes first and stamps the version it arrived at.
        // A header is the one exception.
        // It is what states the version, so nothing has migrated yet.
        // It has to stay legible at every version there will ever be.
        //
        // Nor is it required.
        // A file that states none is version 1.
        // Which is what every replay written before the mechanism says.
        nlohmann::json headerSchema()
        {
            nlohmann::json schema;
            schema["$schema"] = "http://json-schema.org/draft-07/schema#";
            schema["title"] = "antwika replay header";
            schema["type"] = "object";
            schema["additionalProperties"] = false;
            schema["$id"] = "https://antwika.dev/schemas/replay-header";
            schema["required"] = {"magic"}; // GCOVR_EXCL_LINE
            schema["properties"][std::string(detail::kMagicKey)]
                  ["const"] = std::string(detail::kReplayMagic);
            schema["properties"][std::string(kSchemaVersionKey)] =
                countShape();
            schema["properties"]["canvas"] = canvasShape();
            return schema;
        }

        const nlohmann::json_schema::json_validator &headerValidator()
        {
            static const nlohmann::json_schema::json_validator validator(
                headerSchema()); // GCOVR_EXCL_LINE
            return validator;
        }

        // The record's shape is EventSchema.cpp's, used as it stands.
        // It nested as the "items" of the whole-document schema before.
        // A record is a document of its own now, and the same shape.
        // Which is what makes a version 1 file's records readable here.
        const nlohmann::json_schema::json_validator &recordValidator()
        {
            static const nlohmann::json_schema::json_validator validator(
                detail::tickEventShape()); // GCOVR_EXCL_LINE
            return validator;
        }

        // A header opens a file, and a file holds one run.
        // The record schema refuses the member as an unexpected one.
        // This names what actually went wrong instead.
        void requireNotASecondHeader(
            const nlohmann::json &record, const std::size_t ordinal)
        {
            // The key is a named local rather than a temporary.
            // A temporary std::string brings its own branches at -O0.
            const std::string magic(detail::kMagicKey);
            if (!record.is_object() || !record.contains(magic))
            {
                return;
            }

            throw ReplayFormatError(std::format(
                "antwika::replay: record {} is a second header; a replay "
                "holds one run, and two of them appended to one file "
                "would replay as a single session",
                ordinal));
        }

        // Time only ever goes forwards in a recording.
        // A recorder only ever appends to one.
        // Ticks going backwards is two runs interleaved.
        // Or a hand edit that moved a line.
        // Sorting it back into shape would replay a session nobody had.
        void requireTickDoesNotGoBackwards(
            const std::optional<antwika::time::Tick> previous,
            const antwika::time::Tick tick,
            const std::size_t ordinal)
        {
            if (!previous.has_value() || tick >= *previous)
            {
                return;
            }

            throw ReplayFormatError(std::format(
                "antwika::replay: record {} is on tick {}, after a "
                "record on tick {}; a recording's ticks never go "
                "backwards",
                ordinal,
                tick,
                *previous));
        }
    } // namespace

    nlohmann::json replayHeaderToJson(const ReplayHeader &header)
    {
        nlohmann::json encoded;
        encoded[std::string(detail::kMagicKey)] =
            std::string(detail::kReplayMagic);
        encoded[std::string(kSchemaVersionKey)] = header.version;
        if (header.canvas.has_value())
        {
            encoded["canvas"]["width"] = header.canvas->width;
            encoded["canvas"]["height"] = header.canvas->height;
        }
        return encoded;

        // gcov puts this function's cleanup block on its closing brace.
        // Returning an nlohmann::json by value is what creates one.
        // No input reaches it: the function is covered, the brace is not.
        // JsonShapes.cpp reports the same on every one of its four.
    } // GCOVR_EXCL_LINE

    ReplayHeader replayHeaderFromJson(
        const nlohmann::json &j, const MigrationChain &migrations)
    {
        // The version before the schema, deliberately.
        // The stages are parse, version, migrate, validate, decode.
        // A version this build cannot read has to be said outright.
        // Rather than refused for whatever else the header holds.
        ReplayHeader header;
        header.version = documentVersion(j);
        migrations.requireReadable(header.version);

        try
        {
            headerValidator().validate(j);
        }
        catch (const std::exception &error) // GCOVR_EXCL_LINE
        {
            throw ReplayFormatError(
                std::string("antwika::replay: a replay header failed "
                            "schema validation: ")
                + error.what());
        }

        const auto canvas = j.find("canvas");
        if (canvas != j.end())
        {
            header.canvas = gfx::Size{
                .width = canvas->at("width").get<std::uint32_t>(),
                .height = canvas->at("height").get<std::uint32_t>(),
            };
        }
        return header;
    }

    std::vector<event::TickEvent> replayRecordsFromJson(
        const nlohmann::json &records,
        const std::uint32_t version,
        const MigrationChain &migrations)
    {
        if (!records.is_array())
        {
            throw ReplayFormatError(
                "antwika::replay: a replay's records are not a sequence");
        }

        std::vector<event::TickEvent> events;
        events.reserve(records.size());

        std::optional<antwika::time::Tick> previous;
        for (std::size_t index = 0; index < records.size(); ++index)
        {
            const std::size_t ordinal = index + 1;
            requireNotASecondHeader(records[index], ordinal);

            const auto migrated = readVersionedRecord<ReplayFormatError>(
                records[index],
                version,
                migrations,
                recordValidator(),
                std::format(
                    "antwika::replay: record {} failed schema "
                    "validation: ",
                    ordinal));

            auto decoded = migrated.get<event::TickEvent>();
            requireTickDoesNotGoBackwards(
                previous, decoded.tick, ordinal);
            previous = decoded.tick;

            events.push_back(std::move(decoded));
        }
        return events;
    } // GCOVR_EXCL_LINE

    ReplayDocument replayFromJson(
        const nlohmann::json &j, const MigrationChain &migrations)
    {
        if (!j.is_object())
        {
            throw ReplayFormatError(
                "antwika::replay: a whole-document replay is not a JSON "
                "object");
        }

        // A version 1 document minus its log is exactly a header.
        // Each element of that log is exactly a record.
        // Which makes the two shapes one format rather than two.
        nlohmann::json header = j;
        header.erase(std::string(detail::kLegacyEventsKey));

        const ReplayHeader read =
            replayHeaderFromJson(header, migrations);

        ReplayDocument document;
        document.canvas = read.canvas;
        document.events = replayRecordsFromJson(
            j.value(
                std::string(detail::kLegacyEventsKey), nlohmann::json()),
            read.version,
            migrations);
        return document;
    } // GCOVR_EXCL_LINE

} // namespace antwika::replay
