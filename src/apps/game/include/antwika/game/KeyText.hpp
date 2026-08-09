#pragma once

#include <optional>

#include <antwika/input/Key.hpp>
#include <antwika/ui/Keyboard.hpp>

#include <antwika/console/Typing.hpp>

#include "antwika/game/KeyboardLayout.hpp"

namespace antwika::game
{

    [[nodiscard]] std::optional<antwika::ui::Key> uiKeyFor(
        antwika::input::Key key, bool shift) noexcept;

    using antwika::console::typedCharacterFor;

}
