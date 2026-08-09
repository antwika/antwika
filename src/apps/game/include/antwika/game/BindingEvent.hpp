#pragma once

#include <string>

#include "antwika/game/KeyBindings.hpp"

namespace antwika::game
{

    [[nodiscard]] std::string bindKeyPayload(KeyBinding binding);

    [[nodiscard]] KeyBinding bindKeyFromPayload(
        const std::string &payload);

}
