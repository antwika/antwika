#pragma once

#include <string>

#include "antwika/game/KeyboardLayout.hpp"

namespace antwika::game
{

    [[nodiscard]] std::string setKeyboardPayload(KeyboardLayout layout);

    [[nodiscard]] KeyboardLayout keyboardFromPayload(
        const std::string &payload);

}
