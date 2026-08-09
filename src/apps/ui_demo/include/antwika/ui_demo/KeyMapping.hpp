#pragma once

#include <optional>

#include <antwika/input/Key.hpp>
#include <antwika/ui/Keyboard.hpp>

namespace antwika::ui_demo
{

    [[nodiscard]] std::optional<antwika::ui::Key> uiKeyFor(
        antwika::input::Key key, bool shift) noexcept;

    [[nodiscard]] char typedCharacterFor(
        antwika::input::Key key, bool shift) noexcept;

}
