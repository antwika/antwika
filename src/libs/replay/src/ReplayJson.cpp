#include "antwika/replay/ReplayJson.hpp"

#include <nlohmann/json-schema.hpp>

#include <cstddef>
#include <cstdint>
#include <format>
#include <limits>
#include <optional>
#include <string>
#include <utility>

#include <antwika/engine/Events.hpp>
#include <antwika/replay/EventJson.hpp>
#include <antwika/schema/JsonSchemas.hpp>
#include <antwika/replay/ReplayFormatError.hpp>
#include <antwika/schema/SchemaVersion.hpp>
#include <antwika/schema/VersionedDocument.hpp>
#include <antwika/time/Tick.hpp>

#include "EventSchema.hpp"
#include "ReplayFormat.hpp"

namespace antwika::replay
{

    using schema::boundedCountSchema;
    using schema::countSchema;
    using schema::documentVersion;
    using schema::kSchemaVersionKey;
    using schema::readVersionedRecord;

    namespace
    {
        nlohmann::json canvasSchema()
        {
            const nlohmann::json extent = boundedCountSchema(
                std::numeric_limits<std::uint32_t>::max());

            nlohmann::json shape;
            shape["type"] = "object";
            shape["additionalProperties"] = false;
            shape["required"] = {"width", "height"}; // GCOVR_EXCL_LINE
            shape["properties"]["width"] = extent;
            shape["properties"]["height"] = extent;
            return shape;
        }

        nlohmann::json headerSchema()
        {
            nlohmann::json schema;
            schema["$schema"] = "http://json-schema.org/draft-07/schema#";
            schema["title"] = "antwika replay header";
            schema["type"] = "object";
            schema["$id"] = "https://antwika.dev/schemas/replay-header";
            schema["required"] = {"magic"}; // GCOVR_EXCL_LINE
            schema["properties"][std::string(detail::kMagicKey)]
                  ["const"] = std::string(detail::kReplayMagic);
            schema["properties"][std::string(kSchemaVersionKey)] =
                countSchema();
            schema["properties"]["canvas"] = canvasSchema();
            return schema;
        }

        const nlohmann::json_schema::json_validator &headerValidator()
        {
            static const nlohmann::json_schema::json_validator validator(
                headerSchema()); // GCOVR_EXCL_LINE
            return validator;
        }

        const nlohmann::json_schema::json_validator &recordValidator()
        {
            static const nlohmann::json_schema::json_validator validator(
                detail::tickEventSchema()); // GCOVR_EXCL_LINE
            return validator;
        }

        void requireNotASecondHeader(
            const nlohmann::json &record, const std::size_t ordinal)
        {
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

        void requireTickDoesNotGoBackwards(
            const std::optional<antwika::time::Tick> previousTick,
            const antwika::time::Tick tick,
            const std::size_t ordinal)
        {
            if (!previousTick.has_value() || tick >= *previousTick)
            {
                return;
            }

            throw ReplayFormatError(std::format(
                "antwika::replay: record {} is on tick {}, after a "
                "record on tick {}; a recording's ticks never go "
                "backwards",
                ordinal,
                tick,
                *previousTick));
        }
    }

    nlohmann::json replayHeaderToJson(const ReplayHeader &header)
    {
        nlohmann::json headerJson;
        headerJson[std::string(detail::kMagicKey)] =
            std::string(detail::kReplayMagic);
        headerJson[std::string(kSchemaVersionKey)] = header.version;
        if (header.canvasSize.has_value())
        {
            headerJson["canvas"]["width"] = header.canvasSize->width;
            headerJson["canvas"]["height"] = header.canvasSize->height;
        }
        return headerJson;

    } // GCOVR_EXCL_LINE

    ReplayHeader replayHeaderFromJson(
        const nlohmann::json &j, const MigrationChain &migrations)
    {
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
            header.canvasSize = geometry::Size{
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

        std::optional<antwika::time::Tick> previousTick;
        for (std::size_t index = 0; index < records.size(); ++index)
        {
            const std::size_t ordinal = index + 1;
            requireNotASecondHeader(records[index], ordinal);

            const auto migratedRecord = readVersionedRecord<ReplayFormatError>(
                records[index],
                version,
                migrations,
                recordValidator(),
                std::format(
                    "antwika::replay: record {} failed schema "
                    "validation: ",
                    ordinal));

            auto decodedTickEvent = migratedRecord.get<event::TickEvent>();

            if (decodedTickEvent.event.name == antwika::engine::events::kTick)
            {
                throw ReplayFormatError(std::format(
                    "antwika::replay: record {} is named \"{}\"; the "
                    "engine regenerates that event, and a regenerated "
                    "event is never replay input",
                    ordinal,
                    antwika::engine::events::kTick));
            }

            requireTickDoesNotGoBackwards(
                previousTick, decodedTickEvent.tick, ordinal);
            previousTick = decodedTickEvent.tick;

            events.push_back(std::move(decodedTickEvent));
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

        nlohmann::json header = j;
        header.erase(std::string(detail::kLegacyEventsKey));

        const ReplayHeader readHeader =
            replayHeaderFromJson(header, migrations);

        ReplayDocument document;
        document.canvasSize = readHeader.canvasSize;
        document.events = replayRecordsFromJson(
            j.value(
                std::string(detail::kLegacyEventsKey), nlohmann::json()),
            readHeader.version,
            migrations);
        return document;
    } // GCOVR_EXCL_LINE

}
