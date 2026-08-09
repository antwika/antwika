#include "antwika/game/KeyboardLayout.hpp"

#include <array>

namespace antwika::game
{

    namespace
    {
        constexpr std::array<MessageId, kKeyboardLayoutCount> kLabels{
            MessageId::KeyboardEnglish, MessageId::KeyboardSwedish};
    }

    MessageId keyboardLayoutLabel(KeyboardLayout layout) noexcept
    {
        return kLabels[keyboardLayoutIndex(layout)];
    }

}
