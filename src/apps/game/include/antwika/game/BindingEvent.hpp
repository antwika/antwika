#pragma once

#include <string>

#include "antwika/game/KeyBindings.hpp"

namespace antwika::game
{

    /**
     * @brief Encode one binding as a game.bind_key payload.
     *
     * Both halves are written as their persisted names rather than as
     * numbers, for the reason InputEventCodec writes key names rather
     * than scancodes: a recording is read by builds this one has never
     * met, and an enumerator's position is not a promise either of them
     * made.
     *
     * @param binding The action and the key it answers to.
     * @return The payload, a JSON object of two strings.
     */
    [[nodiscard]] std::string bindKeyPayload(KeyBinding binding);

    /**
     * @brief Decode a game.bind_key payload.
     * @param payload The event's raw payload string.
     * @return The binding it names.
     * @throws OptionsFormatError If the payload is not valid JSON, is
     * not this shape, or names an action or a key this build does not
     * know.
     */
    [[nodiscard]] KeyBinding bindKeyFromPayload(
        const std::string &payload);

} // namespace antwika::game
