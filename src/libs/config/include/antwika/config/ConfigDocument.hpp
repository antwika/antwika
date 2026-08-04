#pragma once

#include <cstdint>
#include <fstream>
#include <string_view>
#include <istream>
#include <optional>
#include <ostream>
#include <string>

#include <nlohmann/json-schema.hpp>
#include <nlohmann/json.hpp>

#include <antwika/replay/MigrationChain.hpp>
#include <antwika/replay/VersionedDocument.hpp>

#include "antwika/config/Format.hpp"

namespace antwika::config
{

    /**
     * @brief Begin the schema every config document is validated
     * against.
     *
     * An object refusing unknown members, requiring the format's magic
     * and describing -- without requiring -- its version: a document
     * without one is read as version 1, and by the time a schema runs
     * the document has been migrated, so the only version it may still
     * carry is the current one. The caller adds one property per
     * member its format holds and builds its validator from the
     * result.
     *
     * Unknown members are refused rather than ignored on purpose: a
     * misspelt member silently skipped would be a rebalance that never
     * took.
     *
     * @param format The magic and version the schema pins.
     * @param title What the schema calls the document in a refusal.
     * @return The schema, ready for the caller's properties.
     */
    [[nodiscard]] nlohmann::json documentSchema(
        Format format, std::string_view title);

    /**
     * @brief The shape of a whole number a format bounds both ways.
     *
     * The bounds are what stop nlohmann narrowing in silence --
     * `get<T>()` takes the low bytes of anything wider without a word
     * -- and what refuse a value the field's meaning excludes, a
     * zero-tick period or a negative cost, beside the parse that would
     * admit it.
     *
     * @param minimum The smallest value a document may state.
     * @param maximum The largest value a document may state.
     * @return The schema fragment.
     */
    [[nodiscard]] nlohmann::json wholeShape(
        std::int64_t minimum, std::int64_t maximum);

    /**
     * @brief Begin a document of a format.
     * @param format The magic and version to stamp.
     * @return The document, stating both; the caller adds its members.
     */
    [[nodiscard]] nlohmann::json newDocument(Format format);

    /**
     * @brief Bring a parsed config document to the current version and
     * check it against the caller's schema.
     *
     * The middle of `parse -> read version -> migrate -> validate ->
     * decode`: replay::readVersionedDocument on this module's error
     * type, so every application's config refuses a document from a
     * newer build the same way. Parsing is parseConfig()'s and
     * decoding is the caller's, that being where formats differ.
     *
     * @param document The parsed document.
     * @param migrations The chain that brings it to the current
     * version, constructed and injected by the format that owns it.
     * @param validator The one schema for that current version.
     * @param whatFailed What to say ahead of the validator's own
     * message; the format's name and what it was reading.
     * @return The migrated document, ready to decode.
     * @throws ConfigFormatError If the document states a version this
     * build cannot reach the current one from, or fails the schema.
     */
    [[nodiscard]] nlohmann::json migrated(
        const nlohmann::json &document,
        const replay::MigrationChain &migrations,
        const nlohmann::json_schema::json_validator &validator,
        std::string_view whatFailed);

    /**
     * @brief Read a member a document may leave unstated.
     *
     * Every config member is optional, and an absent one means the
     * caller's default rather than an error: a config stating one
     * number is a one-line rebalance, not a restatement of every
     * default it leaves alone. The schema has already refused any
     * wrong shape, so an absent member is the only branch left here.
     *
     * @tparam WholeT The whole-number type the member decodes into.
     * @param document The migrated, validated document.
     * @param name The member to look for.
     * @param fallback What an unstated member means.
     * @return The member's value, or the fallback.
     */
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

    /**
     * @brief Parse a config document off a stream.
     * @param in Holds the document.
     * @return The parsed document, ready for migrated().
     * @throws ConfigFormatError If the stream does not hold JSON.
     */
    [[nodiscard]] nlohmann::json parseConfig(std::istream &in);

    /**
     * @brief Write a config document to a stream.
     *
     * Two spaces, one member a line: enough to diff a rebalance
     * against the defaults it changes.
     *
     * @param document The document to write.
     * @param out Receives it.
     */
    void writeConfig(const nlohmann::json &document, std::ostream &out);

    /**
     * @brief Parse the config file an installation carries, if any.
     *
     * **A missing file is an ordinary install**, not an error: it is
     * the caller's defaults, which is why this answers nothing rather
     * than throwing and leaves what "defaults" means to the format
     * that owns one. Anything wrong with a file that is there is
     * refused rather than repaired.
     *
     * @param path Where the file would be.
     * @return The parsed document, or nothing when the file is not
     * there.
     * @throws ConfigFormatError If a file is there and does not hold
     * JSON.
     */
    [[nodiscard]] std::optional<nlohmann::json> parseConfigFile(
        const std::string &path);

    /**
     * @brief Bring a document up and validate it, reporting failures
     * as the caller's own error type.
     *
     * migrated()'s general form: a save, an options file and a high
     * score are read exactly the same way and differ only in what a
     * bad one is called.
     *
     * @tparam ErrorT What this format reports a bad document as.
     * @param document The parsed document.
     * @param migrations The chain that brings it to the current
     * version.
     * @param validator The one schema for that version.
     * @param whatFailed What to say ahead of the validator's message.
     * @return The migrated document, ready to decode.
     * @throws ErrorT If it states an unreachable version or fails the
     * schema.
     */
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

    /**
     * @brief Parse a document off a stream.
     * @tparam ErrorT What this format reports bad JSON as.
     * @param in Holds the document.
     * @return The parsed document.
     * @throws ErrorT If the stream does not hold JSON.
     */
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

    /**
     * @brief Parse the document a file holds, if the file is there.
     *
     * **A missing file is not a malformed one.** It is a first run,
     * and unchecked it would reach the parser as an empty stream,
     * which reports "you have never had one of these" as corruption.
     *
     * @tparam ErrorT What this format reports bad JSON as.
     * @param path Where the file would be.
     * @return The parsed document, or nothing when it is not there.
     * @throws ErrorT If a file is there and does not hold JSON.
     */
    template <typename ErrorT>
    [[nodiscard]] std::optional<nlohmann::json> parseFileAs(
        const std::string &path)
    {
        std::ifstream file(path);

        if (!file.is_open())
        {
            return std::nullopt;
        }

        return parseAs<ErrorT>(file);
    }

    /**
     * @brief Write a document out, replacing whatever was there.
     *
     * The write is flushed here rather than by the destructor, which
     * cannot report anything: a full disk fails on the flush and not
     * on the open.
     *
     * @tparam ErrorT What this format reports a failed write as.
     * @param document What to write.
     * @param path Where to write it.
     * @throws ErrorT If the file cannot be opened or written.
     */
    template <typename ErrorT>
    void writeDocumentFileAs(
        const nlohmann::json &document, const std::string &path)
    {
        std::ofstream file(path);
        if (!file.is_open())
        {
            throw ErrorT("antwika: could not open to write: " + path);
        }

        writeConfig(document, file);

        file.flush();
        if (!file)
        {
            throw ErrorT("antwika: could not write: " + path);
        }
    }

} // namespace antwika::config
