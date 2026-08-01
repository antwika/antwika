#pragma once

#include <cstdint>
#include <istream>
#include <ostream>
#include <string_view>

#include <nlohmann/json.hpp>

#include <antwika/replay/MigrationChain.hpp>

#include "antwika/companion/Pet.hpp"

namespace antwika::companion
{

    using antwika::replay::MigrationChain;

    /**
     * @brief What every document of this format says it is.
     *
     * Checked before anything else is read, so a replay or a game save
     * handed to this loader is refused as the wrong kind of file rather
     * than as a companion with every member missing -- which matters
     * because all three state their version in the same member, and the
     * magic is the only thing telling them apart.
     */
    inline constexpr std::string_view kSaveMagic = "antwika-companion";

    /**
     * @brief Which revision of the companion format this build writes.
     *
     * Version 1, and it starts there rather than at zero or at nothing,
     * because a document written without a version can only be dated by
     * guessing. Stated in antwika::replay::kSchemaVersionKey --
     * "version" -- the one member every persisted document in this code
     * base carries its version in.
     *
     * **A bump means a companion written by an older build no longer
     * satisfies this build's schema, or satisfies it and means something
     * else.** Adding an optional member is not that and needs no bump;
     * renaming one, requiring one that was optional, narrowing what a
     * number may be, or reinterpreting a value all are. A bump takes an
     * IMigration from N to N+1 added to standardPetMigrations(), and a
     * test that loads a hand-written version-N document and asserts what
     * it becomes. See docs/schema-versioning.md for the whole rule.
     */
    inline constexpr std::uint32_t kSaveFormatVersion = 1;

    /**
     * @brief Build the migration chain for the companion format.
     *
     * The companion's answer to standardReplayMigrations(), and the
     * whole reason MigrationChain is generic over the document: this
     * names its own list, its own current version and nothing else.
     * There is no registry, so this chain and the replay's cannot see
     * each other.
     * Empty today, because the format is still at version 1 -- a
     * factory rather than a constant so that adding the first migration
     * changes one function and nothing else.
     *
     * @return The chain.
     */
    [[nodiscard]] MigrationChain standardPetMigrations();

    /**
     * @brief Encode a companion as one JSON document.
     * @param memory What the simulation holds.
     * @return The document.
     */
    [[nodiscard]] nlohmann::json petMemoryToJson(const PetMemory &memory);

    /**
     * @brief Decode a companion from one JSON document.
     *
     * Reads it as `parse -> read version -> migrate -> validate ->
     * decode`, migrating before validating so that exactly one schema
     * exists rather than one per revision.
     *
     * @param document The parsed document.
     * @return What the simulation holds.
     * @throws SaveFormatError If the document is not a companion this
     * build can read.
     */
    [[nodiscard]] PetMemory petMemoryFromJson(
        const nlohmann::json &document);

    /**
     * @brief Write a companion to a stream.
     *
     * Indented, unlike a recorded replay: a companion is a dozen lines
     * somebody may well want to read or hand-edit, where a recording is
     * a long machine-written log nobody opens.
     *
     * @param memory What the simulation holds.
     * @param out The stream to write it to.
     */
    void writePetMemory(const PetMemory &memory, std::ostream &out);

    /**
     * @brief Read a companion from a stream.
     *
     * A stream rather than a path, for antwika::gfx::PngReader's two
     * reasons: nothing here opens a file, so every refusal this can
     * produce is reachable from bytes in memory and provable with no
     * fixture on disk.
     *
     * @param in The stream to read from.
     * @return What the simulation holds.
     * @throws SaveFormatError If the stream is not valid JSON, or is not
     * a companion this build can read.
     */
    [[nodiscard]] PetMemory readPetMemory(std::istream &in);

} // namespace antwika::companion
