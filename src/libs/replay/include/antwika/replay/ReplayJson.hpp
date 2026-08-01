#pragma once

#include <nlohmann/json.hpp>

#include <optional>
#include <vector>

#include <antwika/event/TickEvent.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/replay/MigrationChain.hpp>
#include <antwika/replay/ReplayDocument.hpp>

namespace antwika::replay
{

    /**
     * @brief Read a parsed replay document: its version, then a
     * migration to the current one, then the schema, then the decode.
     *
     * The last four stages of `parse -> read version -> migrate ->
     * validate -> decode`, exactly as game::saveGameFromJson() and
     * companion::petMemoryFromJson() are; the first three are
     * readVersionedDocument()'s.
     * Validating after migrating is what lets one schema exist rather
     * than one per revision of the format.
     *
     * @param j The JSON value to read.
     * @param migrations The chain bringing an older document up to
     * kReplayDocumentVersion; standardReplayMigrations() unless a
     * caller injects another.
     * @return The decoded document, its events in the order they
     * occurred, and its canvas set only if the document carries one.
     * @throws ReplayFormatError If j does not match the schema.
     * @throws SchemaVersionError If j states a version this build
     * cannot bring to the current one.
     */
    [[nodiscard]] ReplayDocument replayFromJson(
        const nlohmann::json &j, const MigrationChain &migrations);

    /**
     * @brief Serialize a sequence of tick events to a JSON value
     * matching the replay-document schema.
     * @param events The events to serialize, in the order they occurred.
     * @param canvas The canvas the run laid its input out against; a
     * document with nothing to say here carries no "canvas" member.
     * @return The encoded JSON value.
     *
     * Takes the pieces rather than a ReplayDocument, which reading
     * returns, because a caller writing already holds the events and
     * assembling a document only to encode it would copy every one.
     */
    [[nodiscard]] nlohmann::json replayToJson(
        const std::vector<TickEvent> &events,
        std::optional<gfx::Size> canvas = std::nullopt);

} // namespace antwika::replay
