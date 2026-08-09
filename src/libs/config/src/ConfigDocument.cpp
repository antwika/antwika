#include "antwika/config/ConfigDocument.hpp"

#include <string>

#include <antwika/io/File.hpp>

#include <antwika/replay/SchemaVersion.hpp>
#include <antwika/replay/VersionedDocument.hpp>

#include "antwika/config/ConfigFormatError.hpp"

namespace antwika::config
{

    namespace
    {
        constexpr int kIndent = 2;
    }

    nlohmann::json documentSchema(Format format, std::string_view title)
    {
        nlohmann::json schema;
        schema["$schema"] = "http://json-schema.org/draft-07/schema#";
        schema["title"] = std::string(title);
        schema["type"] = "object";
        schema["additionalProperties"] = false;

        schema["required"] = {"magic"}; // GCOVR_EXCL_LINE
        schema["properties"]["magic"]["const"] =
            std::string(format.magic);
        schema["properties"][std::string(replay::kSchemaVersionKey)]
              ["const"] = format.version;
        return schema;

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
        auto file = io::openToReadIfPresent(path);

        if (!file.has_value())
        {
            return std::nullopt;
        }

        return parseConfig(*file);
    }

}
