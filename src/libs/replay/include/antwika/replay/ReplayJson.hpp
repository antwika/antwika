#pragma once

#include <vector>

#include <nlohmann/json.hpp>

#include <antwika/event/TickEvent.hpp>

namespace antwika::replay
{

    using antwika::event::TickEvent;

    /**
     * @brief Validate a JSON value against the replay-document schema,
     * then deserialize it.
     * @param j The JSON value to validate and deserialize.
     * @return The decoded events, in the order they occurred.
     * @throws ReplayFormatError If j does not match the schema.
     */
    [[nodiscard]] std::vector<TickEvent> replayFromJson(
        const nlohmann::json &j);

    /**
     * @brief Serialize a sequence of tick events to a JSON value
     * matching the replay-document schema.
     * @param events The events to serialize, in the order they occurred.
     * @return The encoded JSON value.
     */
    [[nodiscard]] nlohmann::json replayToJson(
        const std::vector<TickEvent> &events);

} // namespace antwika::replay
