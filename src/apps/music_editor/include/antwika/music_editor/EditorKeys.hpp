#pragma once

#include <optional>

#include <antwika/input/Key.hpp>
#include <antwika/ui/Keyboard.hpp>

namespace antwika::music_editor
{

    /**
     * @brief Translate a key edge into the one antwika::ui acts on.
     *
     * The application's half of the seam antwika::ui leaves open: the
     * library names the keys it has a meaning for and reads no device.
     *
     * Escape is deliberately **not** Cancel here.
     * A field that gave up on what was typed would throw away the line
     * being written, and this editor has nothing else it could mean.
     *
     * @param key The key that went down.
     * @param shift Whether shift was held, which tells Tab from
     * Shift+Tab.
     * @return What the UI should be told, or nothing for a key it has
     * no meaning for.
     */
    [[nodiscard]] std::optional<antwika::ui::Key> uiKeyFor(
        antwika::input::Key key, bool shift) noexcept;

    /**
     * @brief Get the character a key types.
     *
     * A layout written down rather than asked of a window system, so a
     * recording holds the symbolic key and the character comes back
     * identically under any backend on any keyboard.
     *
     * **Every character the mini-notation uses is reachable**, which is
     * the one requirement this table has that ui_demo's does not:
     * brackets, angles, star, slash, percent, bang, query, parentheses,
     * comma and tilde are what a pattern is written with, and a layout
     * missing one would make a form of the grammar untypeable.
     *
     * @param key The key that went down.
     * @param shift Whether shift was held.
     * @return The character, or '\0' when the key types none.
     */
    [[nodiscard]] char typedCharacterFor(
        antwika::input::Key key, bool shift) noexcept;

} // namespace antwika::music_editor
