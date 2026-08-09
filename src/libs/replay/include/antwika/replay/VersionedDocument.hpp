#pragma once

#include <nlohmann/json-schema.hpp>
#include <nlohmann/json.hpp>

#include <exception>
#include <string>
#include <string_view>
#include <type_traits>

#include <antwika/replay/DocumentDepth.hpp>
#include <antwika/replay/MigrationChain.hpp>
#include <antwika/replay/SchemaVersionError.hpp>

namespace antwika::replay
{

    template <typename ErrorT>
    [[nodiscard]] nlohmann::json readVersionedDocument(
        const nlohmann::json &document,
        const MigrationChain &migrations,
        const nlohmann::json_schema::json_validator &validator,
        std::string_view whatFailed)
    {
        if (nestsTooDeep(document))
        {
            throw ErrorT(
                std::string(whatFailed)
                + "the document nests deeper than this format writes");
        }

        nlohmann::json migrated = document;

        if constexpr (std::is_base_of_v<ErrorT, SchemaVersionError>)
        {
            migrations.migrate(migrated);
        }
        else
        {
            try
            {
                migrations.migrate(migrated);
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
            validator.validate(migrated);
        }
        catch (const std::exception &error) // GCOVR_EXCL_LINE
        {
            throw ErrorT(std::string(whatFailed) + error.what());
        }

        return migrated;
    }

    template <typename ErrorT>
    [[nodiscard]] nlohmann::json readVersionedRecord(
        const nlohmann::json &record,
        std::uint32_t statedVersion,
        const MigrationChain &migrations,
        const nlohmann::json_schema::json_validator &validator,
        std::string_view whatFailed)
    {
        if (nestsTooDeep(record))
        {
            throw ErrorT(
                std::string(whatFailed)
                + "the record nests deeper than this format writes");
        }

        nlohmann::json migrated = record;

        migrations.migrateFrom(migrated, statedVersion);

        try
        {
            validator.validate(migrated);
        }
        catch (const std::exception &error) // GCOVR_EXCL_LINE
        {
            throw ErrorT(std::string(whatFailed) + error.what());
        }

        return migrated;
    }

}
