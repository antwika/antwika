#include "antwika/game/KeyboardLayout.hpp"

namespace antwika::game
{

    namespace
    {
        // Tables rather than switches, for actionName()'s reason.
        constexpr std::array<std::string_view, kKeyboardLayoutCount>
            kNames{"english", "swedish"};

        constexpr std::array<MessageId, kKeyboardLayoutCount> kLabels{
            MessageId::KeyboardEnglish, MessageId::KeyboardSwedish};
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

    MessageId keyboardLayoutLabel(KeyboardLayout layout) noexcept
    {
        return kLabels[keyboardLayoutIndex(layout)];
    }

} // namespace antwika::game
