#pragma once

#include <cstdint>
#include <optional>
#include <string_view>

#include <antwika/input/Key.hpp>
#include <antwika/input/KeyModifiers.hpp>
#include <antwika/ui/Keyboard.hpp>

namespace antwika::music_editor
{

    enum class KeyLayout : std::uint8_t
    {
        Swedish = 0,

        English,
    };

    [[nodiscard]] constexpr KeyLayout enumBound(KeyLayout) noexcept
    {
        return KeyLayout::English;
    }

    [[nodiscard]] std::optional<antwika::ui::Key> uiKeyFor(
        antwika::input::Key key,
        antwika::input::KeyModifiers modifiers) noexcept;

    [[nodiscard]] std::string_view typedTextFor(
        antwika::input::Key key,
        antwika::input::KeyModifiers modifiers,
        KeyLayout layout) noexcept;

    [[nodiscard]] std::string_view nameOf(KeyLayout layout) noexcept;

}
