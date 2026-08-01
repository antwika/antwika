#pragma once

#include <cstdint>
#include <istream>
#include <ostream>
#include <string_view>

#include <nlohmann/json.hpp>

#include <antwika/replay/MigrationChain.hpp>

#include "antwika/companion/CompanionMemory.hpp"

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
     * Stated in antwika::replay::kSchemaVersionKey -- "version" -- the
     * one member every persisted document in this code base carries its
     * version in.
     *
     * Version 1 held a companion with two needs and a happiness meter it
     * died of. Version 2 is the same file describing an animal whose
     * *energy* is its life: it gained `fun`, `energy`, `day`, `plays`
     * and `collapses`, and `disturbed` became `woken`. That is a
     * reinterpretation of what the document means rather than an
     * addition to it, which is exactly what the rule calls breaking.
     * Version 3 added the lineage the file keeps across companions,
     * `generation` and `bestTicks`, required rather than optional, and
     * so bumps it again.
     *
     * **A bump means a companion written by an older build no longer
     * satisfies this build's schema, or satisfies it and means something
     * else.** Adding an optional member is not that and needs no bump.
     * A bump takes an IMigration from N to N+1 added to
     * standardPetMigrations(), and a test that loads a hand-written
     * version-N document and asserts what it becomes. See
     * docs/schema-versioning.md for the whole rule.
     */
    inline constexpr std::uint32_t kSaveFormatVersion = 3;

    /**
     * @brief Build the migration chain for the companion format.
     *
     * The companion's answer to standardReplayMigrations(), and the
     * whole reason MigrationChain is generic over the document: this
     * names its own list, its own current version and nothing else.
     * There is no registry, so this chain and the replay's cannot see
     * each other.
     *
     * @return The chain, from version 1 to the current one.
     */
    [[nodiscard]] MigrationChain standardPetMigrations();

    /**
     * @brief Encode a companion and its lineage as one JSON document.
     * @param memory What the session holds.
     * @return The document.
     */
    [[nodiscard]] nlohmann::json companionMemoryToJson(
        const CompanionMemory &memory);

    /**
     * @brief Decode a companion and its lineage from one JSON document.
     *
     * Reads it as `parse -> read version -> migrate -> validate ->
     * decode`, migrating before validating so that exactly one schema
     * exists rather than one per revision.
     *
     * @param document The parsed document.
     * @return What the session holds.
     * @throws SaveFormatError If the document is not a companion this
     * build can read.
     */
    [[nodiscard]] CompanionMemory companionMemoryFromJson(
        const nlohmann::json &document);

    /**
     * @brief Write a companion to a stream.
     *
     * Indented, unlike a recorded replay: a companion is a couple of
     * dozen lines somebody may well want to read or hand-edit, where a
     * recording is a long machine-written log nobody opens.
     *
     * @param memory What the session holds.
     * @param out The stream to write it to.
     */
    void writeCompanionMemory(
        const CompanionMemory &memory, std::ostream &out);

    /**
     * @brief Read a companion from a stream.
     *
     * A stream rather than a path, for antwika::gfx::PngReader's two
     * reasons: nothing here opens a file, so every refusal this can
     * produce is reachable from bytes in memory and provable with no
     * fixture on disk.
     *
     * @param in The stream to read from.
     * @return What the session holds.
     * @throws SaveFormatError If the stream is not valid JSON, or is not
     * a companion this build can read.
     */
    [[nodiscard]] CompanionMemory readCompanionMemory(std::istream &in);

} // namespace antwika::companion
