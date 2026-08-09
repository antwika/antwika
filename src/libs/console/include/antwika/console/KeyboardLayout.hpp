#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>

namespace antwika::console
{

    enum class KeyboardLayout : std::uint8_t
    {
        English = 0,

        Swedish,
    };

    inline constexpr std::array<KeyboardLayout, 2> kKeyboardLayouts{
        KeyboardLayout::English, KeyboardLayout::Swedish};

    inline constexpr std::size_t kKeyboardLayoutCount =
        kKeyboardLayouts.size();

    inline constexpr KeyboardLayout kDefaultKeyboardLayout =
        KeyboardLayout::Swedish;

    [[nodiscard]] constexpr std::size_t keyboardLayoutIndex(
        KeyboardLayout layout) noexcept
    {
        return static_cast<std::size_t>(layout);
    }

    [[nodiscard]] std::string_view keyboardLayoutName(
        KeyboardLayout layout) noexcept;

    [[nodiscard]] std::optional<KeyboardLayout> keyboardLayoutFromName(
        std::string_view name) noexcept;

}
