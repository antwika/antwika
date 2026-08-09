#pragma once

#include <nlohmann/json-schema.hpp>
#include <nlohmann/json.hpp>

#include <istream>
#include <optional>
#include <ostream>
#include <string>
#include <string_view>
#include <utility>

#include <antwika/replay/MigrationChain.hpp>

#include "antwika/config/ConfigDocument.hpp"
#include "antwika/config/ConfigFormatError.hpp"
#include "antwika/config/Format.hpp"

namespace antwika::config
{

    template <typename ConfigT>
    struct FormatSpec final
    {
        Format format;

        std::string_view title;

        std::string_view whatFailed;

        void (*members)(nlohmann::json &schema);

        void (*encode)(const ConfigT &config, nlohmann::json &out);

        ConfigT (*decode)(const nlohmann::json &document);

        replay::MigrationChain (*migrations)();
    };

    template <typename ConfigT, typename ErrorT = ConfigFormatError>
    class FileFormat final
    {
    public:
        explicit FileFormat(FormatSpec<ConfigT> spec)
            : spec(std::move(spec)),
              validator(describedSchema(this->spec))
        {
        }

        FileFormat(const FileFormat &) = delete;
        FileFormat(FileFormat &&) = delete;

        FileFormat &operator=(const FileFormat &) = delete;
        FileFormat &operator=(FileFormat &&) = delete;

        [[nodiscard]] nlohmann::json toJson(const ConfigT &config) const
        {
            auto encoded = newDocument(spec.format);
            spec.encode(config, encoded);
            return encoded;

        } // GCOVR_EXCL_LINE

        [[nodiscard]] ConfigT fromJson(
            const nlohmann::json &document) const
        {
            return spec.decode(migratedAs<ErrorT>(
                document, spec.migrations(), validator, spec.whatFailed));
        }

        void write(const ConfigT &config, std::ostream &out) const
        {
            writeConfig(toJson(config), out);
        }

        [[nodiscard]] ConfigT read(std::istream &in) const
        {
            return fromJson(parseAs<ErrorT>(in));
        }

        [[nodiscard]] ConfigT loadFile(const std::string &path) const
        {
            const auto document = parseFileAs<ErrorT>(path);

            if (!document.has_value())
            {
                throw ErrorT(
                    "antwika: no such file to read: " + path);
            }

            return fromJson(*document);
        }

        [[nodiscard]] std::optional<ConfigT> loadFileIfPresent(
            const std::string &path) const
        {
            const auto document = parseFileAs<ErrorT>(path);

            if (!document.has_value())
            {
                return std::nullopt;
            }

            return fromJson(*document);
        }

        void storeFile(
            const ConfigT &value, const std::string &path) const
        {
            writeDocumentFileAs<ErrorT>(toJson(value), path);
        }

        [[nodiscard]] ConfigT loadFileOrDefaults(
            const std::string &path) const
        {
            const auto document = parseFileAs<ErrorT>(path);

            if (!document.has_value())
            {
                return ConfigT{};
            }

            return fromJson(*document);
        }

    private:
        [[nodiscard]] static nlohmann::json describedSchema(
            const FormatSpec<ConfigT> &spec)
        {
            auto schema = documentSchema(spec.format, spec.title);
            spec.members(schema);
            return schema;
        } // GCOVR_EXCL_LINE

        FormatSpec<ConfigT> spec;
        nlohmann::json_schema::json_validator validator;
    };

}
