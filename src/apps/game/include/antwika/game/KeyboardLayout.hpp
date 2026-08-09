#pragma once

#include <antwika/console/KeyboardLayout.hpp>

#include "antwika/game/MessageId.hpp"

namespace antwika::game
{

    using antwika::console::KeyboardLayout;
    using antwika::console::kKeyboardLayouts;
    using antwika::console::kKeyboardLayoutCount;
    using antwika::console::kDefaultKeyboardLayout;
    using antwika::console::keyboardLayoutIndex;
    using antwika::console::keyboardLayoutName;
    using antwika::console::keyboardLayoutFromName;

    [[nodiscard]] MessageId keyboardLayoutLabel(
        KeyboardLayout layout) noexcept;

}
