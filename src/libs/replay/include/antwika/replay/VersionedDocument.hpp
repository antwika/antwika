#pragma once

#include <exception>
#include <string>
#include <string_view>
#include <type_traits>

#include <nlohmann/json-schema.hpp>
#include <nlohmann/json.hpp>

#include <antwika/replay/MigrationChain.hpp>
#include <antwika/replay/SchemaVersionError.hpp>

namespace antwika::replay
{

    /**
     * @brief Bring a parsed document to the current version of its
     * schema and check it against that one schema.
     *
     * The middle of `parse -> read version -> migrate -> validate ->
     * decode`, which is the order every persisted format in this code
     * base is read in and the one thing about it that must not move:
     * validating *after* migrating is what lets exactly one schema exist
     * rather than one per revision, and reading the version first is
     * what stops a document from a newer build being decoded on the
     * strength of happening to satisfy today's schema.
     * Parsing is the caller's, since only the caller knows what it is
     * parsing from; so is decoding, since that is the whole of what
     * makes one format different from another.
     *
     * **Templated on the error type because each format keeps its own.**
     * A caller loading a game save catches game::SaveFormatError and
     * nothing else, so a chain's antwika::replay error may not be let
     * out of that call unchanged -- an interface promising one exception
     * type must not leak another's.
     *
     * The one exception is a format whose own error type SchemaVersionError
     * already narrows, which is antwika::replay's own: there, translating
     * would *widen* what a caller sees, and a caller telling "your file is
     * from a newer release" apart from "your file is corrupt" catches the
     * narrower type first. That case is decided at compile time, so the
     * translation is not something each format has to remember to ask for
     * or to skip.
     *
     * @tparam ErrorT The exception type this format reports a bad
     * document as; must be constructible from a `const char *`.
     * @param document The parsed document, taken by value because it is
     * migrated in place and a caller's copy may not be.
     * @param migrations The chain that brings it to the current version,
     * constructed and injected by the format that owns it.
     * @param validator The one schema for that current version.
     * @param whatFailed What to say ahead of the validator's own
     * message; the format's name and what it was reading.
     * @return The migrated document, ready to decode.
     * @throws ErrorT If the document states a version this build cannot
     * reach the current one from, or fails the schema.
     * @throws SchemaVersionError Instead of ErrorT for the first of
     * those, when ErrorT is a base of it.
     */
    template <typename ErrorT>
    [[nodiscard]] nlohmann::json readVersionedDocument(
        nlohmann::json document,
        const MigrationChain &migrations,
        const nlohmann::json_schema::json_validator &validator,
        std::string_view whatFailed)
    {
        if constexpr (std::is_base_of_v<ErrorT, SchemaVersionError>)
        {
            migrations.migrate(document);
        }
        else
        {
            try
            {
                migrations.migrate(document);
            }
            // GCOVR_EXCL_START
            catch (const SchemaVersionError &error)
            {
                // Carried through rather than rewritten.
                // The chain's message names both versions already.
                throw ErrorT(error.what());
            }
            // GCOVR_EXCL_STOP
        }

        try
        {
            validator.validate(document);
        }
        catch (const std::exception &error) // GCOVR_EXCL_LINE
        {
            throw ErrorT(std::string(whatFailed) + error.what());
        }

        return document;
    }

} // namespace antwika::replay
