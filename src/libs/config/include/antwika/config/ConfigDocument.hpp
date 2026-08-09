#pragma once

#include <nlohmann/json-schema.hpp>
#include <nlohmann/json.hpp>

#include <cstdint>
#include <string_view>
#include <istream>
#include <optional>
#include <ostream>
#include <string>

#include <antwika/io/File.hpp>
#include <antwika/replay/MigrationChain.hpp>
#include <antwika/replay/VersionedDocument.hpp>

#include "antwika/config/Format.hpp"

namespace antwika::config
{

    [[nodiscard]] nlohmann::json documentSchema(
        Format format, std::string_view title);

    [[nodiscard]] nlohmann::json wholeShape(
        std::int64_t minimum, std::int64_t maximum);

    [[nodiscard]] nlohmann::json newDocument(Format format);

    [[nodiscard]] nlohmann::json migrated(
        const nlohmann::json &document,
        const replay::MigrationChain &migrations,
        const nlohmann::json_schema::json_validator &validator,
        std::string_view whatFailed);

    template <typename WholeT>
    [[nodiscard]] WholeT memberOr(
        const nlohmann::json &document,
        const char *name,
        WholeT fallback)
    {
        return document.contains(name)
                   ? document.at(name).get<WholeT>()
                   : fallback;
    }

    [[nodiscard]] nlohmann::json parseConfig(std::istream &in);

    void writeConfig(const nlohmann::json &document, std::ostream &out);

    [[nodiscard]] std::optional<nlohmann::json> parseConfigFile(
        const std::string &path);

    template <typename ErrorT>
    [[nodiscard]] nlohmann::json migratedAs(
        const nlohmann::json &document,
        const replay::MigrationChain &migrations,
        const nlohmann::json_schema::json_validator &validator,
        std::string_view whatFailed)
    {
        return replay::readVersionedDocument<ErrorT>(
            document, migrations, validator, whatFailed);
    }

    template <typename ErrorT>
    [[nodiscard]] nlohmann::json parseAs(std::istream &in)
    {
        nlohmann::json document;
        try
        {
            in >> document;
        }
        catch (const nlohmann::json::exception &error) // GCOVR_EXCL_LINE
        {
            throw ErrorT(
                std::string("antwika: the document is not valid JSON: ")
                + error.what());
        }

        return document;
    }

    template <typename ErrorT>
    [[nodiscard]] std::optional<nlohmann::json> parseFileAs(
        const std::string &path)
    {
        auto file = io::openToReadIfPresent(path);

        if (!file.has_value())
        {
            return std::nullopt;
        }

        return parseAs<ErrorT>(*file);
    }

    template <typename ErrorT>
    void writeDocumentFileAs(
        const nlohmann::json &document, const std::string &path)
    {
        io::writeFileAs<ErrorT>(
            path, "a document", [&document](std::ostream &out) {
                writeConfig(document, out);
            });
    }

}
