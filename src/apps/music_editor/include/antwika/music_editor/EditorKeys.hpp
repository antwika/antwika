#pragma once

#include <optional>
#include <string_view>

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
     * A field that gave up on what was typed would throw away the score
     * being written; Escape is what pauses instead, and the sink reads
     * that key itself rather than through the UI.
     *
     * Tab is not focus either, since there is one thing to type into.
     * It indents, which is what Tab in a code editor is for, and that
     * makes it characters rather than a key with a meaning.
     *
     * @param key The key that went down.
     * @param shift Whether shift was held.
     * @return What the UI should be told, or nothing for a key it has
     * no meaning for.
     */
    [[nodiscard]] std::optional<antwika::ui::Key> uiKeyFor(
        antwika::input::Key key, bool shift) noexcept;

    /**
     * @brief Get the characters a key types.
     *
     * A layout written down rather than asked of a window system, so a
     * recording holds the symbolic key and the characters come back
     * identically under any backend on any keyboard.
     *
     * **Every character the score is written with is reachable**, which
     * is the one requirement this table has that ui_demo's does not:
     * brackets, angles, star, slash, percent, bang, query, parentheses,
     * comma, tilde, and the dollar and colon a voice line opens with
     * are what a score is written from, and a layout missing one would
     * make a form of the grammar untypeable.
     *
     * Characters rather than one character, because Tab indents by two.
     *
     * @param key The key that went down.
     * @param shift Whether shift was held.
     * @return The characters, which outlive every caller, or an empty
     * view when the key types none.
     */
    [[nodiscard]] std::string_view typedTextFor(
        antwika::input::Key key, bool shift) noexcept;

} // namespace antwika::music_editor
