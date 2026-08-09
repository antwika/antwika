#pragma once

#include <optional>

#include <antwika/input/Key.hpp>
#include <antwika/ui/Keyboard.hpp>

#include "antwika/console/KeyboardLayout.hpp"

namespace antwika::console
{

    [[nodiscard]] char typedCharacterFor(
        antwika::input::Key key,
        bool shift,
        KeyboardLayout layout,
        bool alt = false) noexcept;

    [[nodiscard]] std::optional<antwika::ui::Key> consoleKeyFor(
        antwika::input::Key key) noexcept;

}
