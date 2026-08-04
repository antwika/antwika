#pragma once

#include <antwika/console/KeyboardLayout.hpp>

#include "antwika/game/MessageId.hpp"

namespace antwika::game
{

    /**
     * @brief The layout moved to antwika::console with the debug
     * console; the names are kept here on InputFold.hpp's terms, and
     * the caption below stays this application's because a MessageId
     * is each module's own.
     */
    using antwika::console::KeyboardLayout;
    using antwika::console::kKeyboardLayouts;
    using antwika::console::kKeyboardLayoutCount;
    using antwika::console::kDefaultKeyboardLayout;
    using antwika::console::keyboardLayoutIndex;
    using antwika::console::keyboardLayoutName;
    using antwika::console::keyboardLayoutFromName;

    /**
     * @brief Get what a layout is called on screen.
     * @param layout The layout to word.
     * @return The id of the caption, for a Translator to word.
     */
    [[nodiscard]] MessageId keyboardLayoutLabel(
        KeyboardLayout layout) noexcept;

} // namespace antwika::game
