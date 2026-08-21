#pragma once

#include <nlohmann/json-schema.hpp>
#include <nlohmann/json.hpp>

#include <exception>
#include <string>
#include <string_view>
#include <type_traits>

#include <antwika/schema/DocumentDepth.hpp>
#include <antwika/schema/MigrationChain.hpp>
#include <antwika/schema/SchemaVersionError.hpp>

namespace antwika::schema
{

    template <typename ErrorT>
    [[nodiscard]] nlohmann::json readVersionedDocument(
        const nlohmann::json &document,
        const MigrationChain &migrations,
        const nlohmann::json_schema::json_validator &validator,
        std::string_view errorPrefix)
    {
        if (exceedsMaxDepth(document))
        {
            throw ErrorT(
                std::string(errorPrefix)
                + "the document nests deeper than this format writes");
        }

        nlohmann::json migratedJson = document;

        if constexpr (std::is_base_of_v<ErrorT, SchemaVersionError>)
        {
            migrations.migrate(migratedJson);
        }
        else
        {
            try
            {
                migrations.migrate(migratedJson);
            }
            // GCOVR_EXCL_START
            catch (const SchemaVersionError &error)
            {
                throw ErrorT(error.what());
            }
            // GCOVR_EXCL_STOP
        }

        try
        {
            validator.validate(migratedJson);
        }
        catch (const std::exception &error) // GCOVR_EXCL_LINE
        {
            throw ErrorT(std::string(errorPrefix) + error.what());
        }

        return migratedJson;
    }

    template <typename ErrorT>
    [[nodiscard]] nlohmann::json readVersionedRecord(
        const nlohmann::json &record,
        std::uint32_t statedVersion,
        const MigrationChain &migrations,
        const nlohmann::json_schema::json_validator &validator,
        std::string_view errorPrefix)
    {
        if (exceedsMaxDepth(record))
        {
            throw ErrorT(
                std::string(errorPrefix)
                + "the record nests deeper than this format writes");
        }

        nlohmann::json migratedJson = record;

        migrations.migrateFrom(migratedJson, statedVersion);

        try
        {
            validator.validate(migratedJson);
        }
        catch (const std::exception &error) // GCOVR_EXCL_LINE
        {
            throw ErrorT(std::string(errorPrefix) + error.what());
        }

        return migratedJson;
    }

}
