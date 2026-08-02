#pragma once

#include <cstdint>
#include <string_view>

#include <nlohmann/json.hpp>

/**
 * @file
 * @brief The schema versions this library reads and writes, and the one
 * key a versioned document carries them in.
 *
 * Every persisted document in this code base -- a replay today, a save
 * file tomorrow -- says which revision of its schema it was written
 * against, so a reader can refuse a document it does not understand
 * rather than decode it into something plausible and wrong.
 *
 * @section versioning How to bump a version
 *
 * A version is a plain, monotonically increasing integer.
 * There is no minor number: a reader either understands a revision or it
 * does not, and half-understanding is the failure this whole mechanism
 * exists to prevent.
 *
 * A change is *additive* when every document valid under version N is
 * still valid under N+1 and still means the same thing.
 * Adding an optional member is the usual case -- "canvas" was one.
 * An additive change does not need a bump, and did not get one.
 *
 * A change is *breaking* when a document written under N is no longer
 * valid, or is valid but means something else: renaming or removing a
 * member, tightening a constraint, making an optional member required,
 * or reinterpreting an existing value.
 * A breaking change needs a bump, and a bump needs a migration -- one
 * IMigration from N to N+1, registered in the chain
 * standardReplayMigrations() returns, so that every older document still
 * loads.
 * See docs/schema-versioning.md for the whole rule.
 */
namespace antwika::replay
{

    /**
     * @brief The member a versioned document states its schema version
     * in.
     *
     * One name for every document kind in the code base, so a reader can
     * find the version before it knows what it is reading.
     * "version" rather than "schemaVersion" because that is what the
     * replay format has written since it shipped, and renaming it would
     * break every file already on disk for no gain.
     */
    inline constexpr std::string_view kSchemaVersionKey = "version";

    /**
     * @brief The version a document with no version member is taken to
     * be.
     *
     * Version 1 predates this mechanism, so a file from before it has
     * nothing to say on the subject.
     * Reading the absence as 1 is what keeps a third party's older file
     * loading; the checked-in fixtures all state their version outright.
     */
    inline constexpr std::uint32_t kUnversionedDocumentVersion = 1;

    /**
     * @brief The replay schema version ReplayWriter writes into a file's
     * header line and ReplayReader decodes its records at.
     *
     * Version 1 was one JSON object holding an "events" array.
     * Version 2 is JSON Lines: a header line, then one record a line.
     * A version 1 file still loads, because the header members and the
     * record shape are the same in both -- see ReplayReader.
     */
    inline constexpr std::uint32_t kReplayDocumentVersion = 2;

    /**
     * @brief The version of the tick-event schema one record of a replay
     * is written against.
     *
     * Stated here rather than in each record, since a record is repeated
     * thousands of times in one file and its revision is fixed by the
     * header that opens the file.
     * Nothing writes it into a file: it is a number to bump and reason
     * about when the record shape changes, and what a reader actually
     * dispatches on is the header's version beside it.
     */
    inline constexpr std::uint32_t kTickEventSchemaVersion = 1;

    /**
     * @brief Read a document's schema version.
     * @param document The parsed document to look in.
     * @param versionKey The member holding the version; the shared
     * kSchemaVersionKey unless a caller's format says otherwise.
     * @return The stated version, or kUnversionedDocumentVersion when
     * the member is absent.
     * @throws SchemaVersionError If the member is present but is not a
     * non-negative integer that fits in a std::uint32_t.
     *
     * Deliberately does not validate the rest of the document: a reader
     * has to know the version before it knows which schema to check the
     * document against.
     */
    [[nodiscard]] std::uint32_t documentVersion(
        const nlohmann::json &document,
        std::string_view versionKey = kSchemaVersionKey);

} // namespace antwika::replay
