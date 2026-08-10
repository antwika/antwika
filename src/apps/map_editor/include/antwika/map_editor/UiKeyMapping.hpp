#pragma once

#include <optional>

#include <antwika/input/Key.hpp>
#include <antwika/ui/Keyboard.hpp>

namespace antwika::map_editor
{

    [[nodiscard]] std::optional<ui::Key> uiKeyFor(
        input::Key key, bool shift) noexcept;

    [[nodiscard]] char typedCharacterFor(
        input::Key key, bool shift) noexcept;

}
