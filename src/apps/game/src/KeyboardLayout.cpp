#include "antwika/game/KeyboardLayout.hpp"

#include <array>

namespace antwika::game
{

    namespace
    {
        // A table rather than a switch, for actionName()'s reason.
        constexpr std::array<MessageId, kKeyboardLayoutCount> kLabels{
            MessageId::KeyboardEnglish, MessageId::KeyboardSwedish};
    } // namespace

    MessageId keyboardLayoutLabel(KeyboardLayout layout) noexcept
    {
        return kLabels[keyboardLayoutIndex(layout)];
    }

} // namespace antwika::game
