#include "antwika/console/KeyboardLayout.hpp"

namespace antwika::console
{

    namespace
    {
        // Tables rather than switches, for actionName()'s reason.
        constexpr std::array<std::string_view, kKeyboardLayoutCount>
            kNames{"english", "swedish"};

    } // namespace

    std::string_view keyboardLayoutName(KeyboardLayout layout) noexcept
    {
        return kNames[keyboardLayoutIndex(layout)];
    }

    std::optional<KeyboardLayout> keyboardLayoutFromName(
        std::string_view name) noexcept
    {
        for (const auto layout : kKeyboardLayouts)
        {
            if (keyboardLayoutName(layout) == name)
            {
                return layout;
            }
        }

        return std::nullopt;
    }


} // namespace antwika::console
