#include "antwika/config/ConfigDocument.hpp"

#include <fstream>
#include <string>

#include <antwika/replay/SchemaVersion.hpp>
#include <antwika/replay/VersionedDocument.hpp>

#include "antwika/config/ConfigFormatError.hpp"

namespace antwika::config
{

    namespace
    {
        // Two spaces, one member a line.
        // Enough to diff a rebalance against the defaults it changes.
        constexpr int kIndent = 2;
    } // namespace

    nlohmann::json documentSchema(Format format, std::string_view title)
    {
        nlohmann::json schema;
        schema["$schema"] = "http://json-schema.org/draft-07/schema#";
        schema["title"] = std::string(title);
        schema["type"] = "object";
        schema["additionalProperties"] = false;

        // The version member is described but not required.
        // A document without one is read as version 1 instead.
        // By the time a schema runs the document has been migrated.
        // So the only version it may carry is the current one.
        schema["required"] = {"magic"}; // GCOVR_EXCL_LINE
        schema["properties"]["magic"]["const"] =
            std::string(format.magic);
        schema["properties"][std::string(replay::kSchemaVersionKey)]
              ["const"] = format.version;
        return schema;

        // gcov puts the returned json's unwind block on this brace.
        // SaveGame.cpp's moneyShape() explains it at length.
        // No input reaches it.
    } // GCOVR_EXCL_LINE

    nlohmann::json wholeShape(
        std::int64_t minimum, std::int64_t maximum)
    {
        nlohmann::json shape;
        shape["type"] = "integer";
        shape["minimum"] = minimum;
        shape["maximum"] = maximum;
        return shape;
    } // GCOVR_EXCL_LINE

    nlohmann::json newDocument(Format format)
    {
        nlohmann::json document;
        document["magic"] = std::string(format.magic);
        document[std::string(replay::kSchemaVersionKey)] =
            format.version;
        return document;
    } // GCOVR_EXCL_LINE

    nlohmann::json migrated(
        const nlohmann::json &document,
        const replay::MigrationChain &migrations,
        const nlohmann::json_schema::json_validator &validator,
        std::string_view whatFailed)
    {
        return replay::readVersionedDocument<ConfigFormatError>(
            document, migrations, validator, whatFailed);
    }

    nlohmann::json parseConfig(std::istream &in)
    {
        nlohmann::json document;
        try
        {
            in >> document;
        }
        catch (const nlohmann::json::exception &error) // GCOVR_EXCL_LINE
        {
            throw ConfigFormatError(
                std::string(
                    "antwika::config: the config is not valid JSON: ")
                + error.what());
        }

        return document;
    }

    void writeConfig(const nlohmann::json &document, std::ostream &out)
    {
        out << document.dump(kIndent) << '\n';
    }

    std::optional<nlohmann::json> parseConfigFile(
        const std::string &path)
    {
        std::ifstream file(path);

        // A file that is not there is an install nobody has rebalanced.
        // Which is a state rather than a failure.
        if (!file.is_open())
        {
            return std::nullopt;
        }

        return parseConfig(file);
    }

} // namespace antwika::config
