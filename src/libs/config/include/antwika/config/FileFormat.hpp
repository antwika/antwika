#pragma once

#include <istream>
#include <optional>
#include <ostream>
#include <string>
#include <string_view>
#include <utility>

#include <nlohmann/json-schema.hpp>
#include <nlohmann/json.hpp>

#include <antwika/replay/MigrationChain.hpp>

#include "antwika/config/ConfigDocument.hpp"
#include "antwika/config/ConfigFormatError.hpp"
#include "antwika/config/Format.hpp"

namespace antwika::config
{

    /**
     * @brief What one application states about its config format.
     *
     * The four members are exactly what differed between the
     * applications' loaders when each carried its own copy of the
     * plumbing: which format this is, what its schema admits beyond
     * the envelope, and how a config value becomes members and comes
     * back.
     * Everything the loaders shared -- the envelope, the
     * parse -> read version -> migrate -> validate order, the stream
     * and file handling -- lives in FileFormat below and is not
     * theirs to restate.
     *
     * Plain function pointers and views rather than std::function
     * and std::string, deliberately: every application's spec is
     * stateless free functions over literals, a pointer's call has no
     * branch for the coverage gate to chase, and a capturing lambda
     * arriving here would be state the format kept that its config
     * value did not.
     *
     * @tparam ConfigT The value the format reads and writes.
     */
    template <typename ConfigT>
    struct FormatSpec
    {
        /** @brief The magic and current version. */
        Format format;

        /** @brief What the schema calls the document in a refusal. */
        std::string_view title;

        /** @brief What to say ahead of a schema failure's message. */
        std::string_view whatFailed;

        /**
         * @brief Add this format's properties to the envelope schema.
         *
         * Handed the result of documentSchema(); one property per
         * member the format holds.
         */
        void (*members)(nlohmann::json &schema);

        /**
         * @brief State every member onto a stamped document.
         */
        void (*encode)(const ConfigT &config, nlohmann::json &out);

        /**
         * @brief Decode a migrated, validated document.
         *
         * Absent members mean the defaults, which is memberOr()'s
         * job; a rule between two members is refused here, since the
         * schema checks each number alone.
         */
        ConfigT (*decode)(const nlohmann::json &document);

        /**
         * @brief Build the chain that brings an old document up.
         *
         * A factory rather than a chain, because MigrationChain is
         * built per read and adding a migration should change one
         * function in the owning application.
         * A config file's is AppConfigFile's own, since every one of
         * them is empty for the same reason.
         */
        replay::MigrationChain (*migrations)();
    };

    /**
     * @brief One application's config format, with the shared reading
     * and writing plumbing attached.
     *
     * **It serves any versioned JSON document, not only a config.**
     * A save, an options file and a high score are read exactly the
     * same way; what differs between them is the error type each
     * reports, which is why that is the second parameter rather than
     * something baked in. `antwika::config`'s own documents leave it
     * defaulted.
     *
     * The class is the pattern the per-application loaders repeated
     * nine times, said once: a caller declares a FormatSpec and thin
     * free functions forwarding here, and what it took to read a
     * versioned document correctly -- and identically to every other
     * caller -- stops being its concern.
     * A config file declares neither by hand any more, since
     * AppConfigFile.hpp writes both from what one application states;
     * this is what a document that is not a config, or a config that
     * has outgrown that shape, still reaches for.
     *
     * The validator is built once, at construction, from the envelope
     * plus the spec's members; the schema therefore cannot drift from
     * the decode it guards, and a misspelt member is refused with the
     * property list in the message.
     */
    template <typename ConfigT, typename ErrorT = ConfigFormatError>
    class FileFormat final
    {
    public:
        /**
         * @brief Build a format over what one application states.
         * @param spec The application's half of the format.
         */
        explicit FileFormat(FormatSpec<ConfigT> spec)
            : spec(std::move(spec)),
              validator(describedSchema(this->spec))
        {
        }

        FileFormat(const FileFormat &) = delete;
        FileFormat(FileFormat &&) = delete;

        FileFormat &operator=(const FileFormat &) = delete;
        FileFormat &operator=(FileFormat &&) = delete;

        /**
         * @brief Encode a config as a document stating every member.
         * @param config The config to write.
         * @return The document.
         */
        [[nodiscard]] nlohmann::json toJson(const ConfigT &config) const
        {
            auto encoded = newDocument(spec.format);
            spec.encode(config, encoded);
            return encoded;

            // gcov puts the cleanup block on this closing brace.
            // SaveGame.cpp's own encoder explains it at length.
            // No input reaches it.
        } // GCOVR_EXCL_LINE

        /**
         * @brief Decode a config document.
         * @param document The parsed document.
         * @return The config it states, defaults filling the rest.
         * @throws ConfigFormatError If it is not this format, states
         * a version this build cannot reach the current one from, or
         * fails the schema.
         */
        [[nodiscard]] ConfigT fromJson(
            const nlohmann::json &document) const
        {
            return spec.decode(migratedAs<ErrorT>(
                document, spec.migrations(), validator, spec.whatFailed));
        }

        /**
         * @brief Write a config to a stream.
         * @param config The config to write.
         * @param out Receives the document.
         */
        void write(const ConfigT &config, std::ostream &out) const
        {
            writeConfig(toJson(config), out);
        }

        /**
         * @brief Read a config from a stream.
         * @param in Holds the document.
         * @return The config it holds.
         * @throws ConfigFormatError If the stream does not hold one.
         */
        [[nodiscard]] ConfigT read(std::istream &in) const
        {
            return fromJson(parseAs<ErrorT>(in));
        }

        /**
         * @brief Read a value from a file that must be there.
         * @param path Where the file is.
         * @return What it held.
         * @throws ErrorT If the file is missing or is not one of these.
         */
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

        /**
         * @brief Read a value from a file, or nothing when absent.
         * @param path Where the file would be.
         * @return What it held, or nothing at all.
         * @throws ErrorT If a file is there and is not one of these.
         */
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

        /**
         * @brief Write a value to a file, replacing what was there.
         * @param value What to write.
         * @param path Where to write it.
         * @throws ErrorT If the file cannot be opened or written.
         */
        void storeFile(
            const ConfigT &value, const std::string &path) const
        {
            writeDocumentFileAs<ErrorT>(toJson(value), path);
        }

        /**
         * @brief Read the config an installation carries, if any.
         *
         * A missing file is an ordinary install playing the shipped
         * defaults; anything wrong with a file that is there is
         * refused rather than repaired.
         *
         * @param path Where the file would be.
         * @return What it held, or a default-constructed config.
         * @throws ConfigFormatError If a file is there and is not one
         * of these.
         */
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

} // namespace antwika::config
