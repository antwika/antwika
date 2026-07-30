#pragma once

#include <nlohmann/json.hpp>

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
     * @brief Serialize a replay document to a JSON value matching the
     * replay-document schema.
     * @param document The document to serialize.
     * @return The encoded JSON value, carrying a "canvas" member only if
     * the document has a canvas to write.
     */
    [[nodiscard]] nlohmann::json replayToJson(
        const ReplayDocument &document);

} // namespace antwika::replay
