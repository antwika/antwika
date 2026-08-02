#pragma once

#include <nlohmann/json.hpp>

#include <cstdint>
#include <vector>

#include <antwika/event/TickEvent.hpp>
#include <antwika/replay/MigrationChain.hpp>
#include <antwika/replay/ReplayDocument.hpp>
#include <antwika/replay/ReplayHeader.hpp>

namespace antwika::replay
{

    /**
     * @brief Serialize a replay's header line.
     * @param header The version and canvas to state; a header with
     * nothing to say about its canvas carries no "canvas" member.
     * @return The encoded JSON value, one line when dumped compact.
     */
    [[nodiscard]] nlohmann::json replayHeaderToJson(
        const ReplayHeader &header);

    /**
     * @brief Read a parsed replay header: the schema, then the version,
     * then the decode.
     *
     * The header is the one part of a replay that is never migrated, and
     * that is deliberate: it is what states the version, so every build
     * that will ever read this file has to understand it as it stands.
     * It may therefore only grow -- a new member is optional or it is a
     * new format -- and the schema here describes every version at once
     * rather than only the current one.
     *
     * @param j The JSON value to read.
     * @param migrations The chain whose current version the header's is
     * answered against.
     * @return The decoded header, its canvas set only if the file
     * carries one.
     * @throws ReplayFormatError If j does not match the header schema.
     * @throws SchemaVersionError If j states a version this build cannot
     * bring to the current one.
     */
    [[nodiscard]] ReplayHeader replayHeaderFromJson(
        const nlohmann::json &j, const MigrationChain &migrations);

    /**
     * @brief Decode a replay's records, checking what spans them.
     * @param records The parsed records, in file order.
     * @param version The version the file's header stated.
     * @param migrations The chain bringing an older record up to
     * kReplayDocumentVersion; standardReplayMigrations() unless a caller
     * injects another.
     * @return The decoded events, in the order they were recorded.
     * @throws ReplayFormatError If records is not an array, if any
     * record fails the record schema, or if the sequence breaks a rule
     * that no single record can: a tick that goes backwards, or a second
     * header part-way through.
     * @throws SchemaVersionError If the stated version is one this build
     * cannot bring to the current one.
     *
     * **The two checks that used to be the shape of the file.** A whole
     * document put every record inside one array, so a reader that got
     * the document got all of it, in order, once. A file of lines cannot
     * say that by its shape, so it says it here instead -- these are the
     * rules a per-line schema has nowhere to live.
     */
    [[nodiscard]] std::vector<event::TickEvent> replayRecordsFromJson(
        const nlohmann::json &records,
        std::uint32_t version,
        const MigrationChain &migrations);

    /**
     * @brief Read a whole version 1 replay: one JSON object holding its
     * entire event log in an "events" array.
     *
     * The shape every replay was written in before JSON Lines, and the
     * shape a file a third party still has is in.
     * It is read by splitting rather than by a decoder of its own: the
     * object minus its "events" is exactly a header, and each element of
     * that array is exactly a record, so both shapes converge on one
     * pipeline and one in-memory type.
     *
     * @param j The JSON value to read.
     * @param migrations The chain bringing an older record up to
     * kReplayDocumentVersion.
     * @return The decoded document, its events in the order they
     * occurred, and its canvas set only if the document carries one.
     * @throws ReplayFormatError If j does not match the schema.
     * @throws SchemaVersionError If j states a version this build cannot
     * bring to the current one.
     */
    [[nodiscard]] ReplayDocument replayFromJson(
        const nlohmann::json &j, const MigrationChain &migrations);

} // namespace antwika::replay
