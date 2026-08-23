#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>

namespace antwika::input
{

    enum class MouseButton : std::uint8_t
    {
        Left = 0,
        Middle,
        Right,
        X1,
        X2,
    };

    inline constexpr std::size_t kMouseButtonCount =
        static_cast<std::size_t>(MouseButton::X2) + 1;

    [[nodiscard]] constexpr std::size_t getMouseButtonIndex(
        MouseButton button) noexcept
    {
        return static_cast<std::size_t>(button);
    }

    [[nodiscard]] std::string_view toString(MouseButton button);

    [[nodiscard]] MouseButton getMouseButtonFromString(std::string_view name);

}
