#pragma once

#include <nlohmann/json.hpp>

#include <optional>
#include <vector>

#include <antwika/event/TickEvent.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/replay/ReplayDocument.hpp>

namespace antwika::replay
{

    /**
     * @brief Validate a JSON value against the replay-document schema,
     * then deserialize it.
     * @param j The JSON value to validate and deserialize.
     * @return The decoded document, its events in the order they
     * occurred, and its canvas set only if the document carries one.
     * @throws ReplayFormatError If j does not match the schema.
     */
    [[nodiscard]] ReplayDocument replayFromJson(const nlohmann::json &j);

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
