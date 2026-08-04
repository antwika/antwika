#pragma once

#include <string>

#include "antwika/game/KeyboardLayout.hpp"

namespace antwika::game
{

    /**
     * @brief Encode a layout as a game.set_keyboard payload.
     * @param layout The layout the run types by.
     * @return The JSON payload, one "keyboard" member.
     */
    [[nodiscard]] std::string setKeyboardPayload(KeyboardLayout layout);

    /**
     * @brief Decode a game.set_keyboard payload.
     * @param payload The payload to read.
     * @return The layout it names.
     * @throws OptionsFormatError If the payload is not that shape, or
     * names a layout this build does not know.
     */
    [[nodiscard]] KeyboardLayout keyboardFromPayload(
        const std::string &payload);

} // namespace antwika::game
