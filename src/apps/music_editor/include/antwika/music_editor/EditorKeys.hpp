#pragma once

#include <cstdint>
#include <optional>
#include <string_view>

#include <antwika/input/Key.hpp>
#include <antwika/input/KeyModifiers.hpp>
#include <antwika/ui/Keyboard.hpp>

namespace antwika::music_editor
{

    /**
     * @brief Which keyboard the characters are read off.
     *
     * A key arrives here as the place it is on the board rather than as
     * a character -- that is what antwika::input::Key means -- so what
     * it types is this table's answer, and a layout is which table.
     *
     * Two of them, because a score is written on a real keyboard and
     * the same physical key is `;` on one and `ö` on another. Getting
     * that wrong is not cosmetic here: `$:` opens every voice line, and
     * the Swedish board writes that colon where the American one writes
     * a greater-than.
     */
    enum class KeyLayout : std::uint8_t
    {
        /**
         * @brief The Swedish board, which is the default.
         */
        Swedish = 0,

        /**
         * @brief The American board.
         */
        English,
    };

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
     * Held shift turns each arrow into the selecting one beside it, and
     * held control turns C and X into a copy and a cut. **Control and V
     * is not here**: a paste is the characters of a clipboard this
     * library knows nothing about, so the sink types them instead.
     *
     * @param key The key that went down.
     * @param modifiers What was held with it.
     * @return What the UI should be told, or nothing for a key it has
     * no meaning for.
     */
    [[nodiscard]] std::optional<antwika::ui::Key> uiKeyFor(
        antwika::input::Key key,
        antwika::input::KeyModifiers modifiers) noexcept;

    /**
     * @brief Get the characters a key types.
     *
     * A layout written down rather than asked of a window system, so a
     * recording holds the symbolic key and the characters come back
     * identically under any backend on any machine -- including one
     * whose window system is set to a different board than the one this
     * is reading by.
     *
     * **Every character the score is written with is reachable**, which
     * is the one requirement this table has that ui_demo's does not:
     * brackets, angles, star, slash, percent, bang, query, parentheses,
     * comma, tilde, and the dollar and colon a voice line opens with
     * are what a score is written from, and a layout missing one would
     * make a form of the grammar untypeable. On the Swedish board four
     * of them are on the right-hand alt key, which is where that board
     * really keeps them.
     *
     * A key whose character this window cannot draw types nothing:
     * antwika::gfx covers printable ASCII, so the Swedish board's own
     * letters and its dead keys are absent rather than written as
     * something no font here has a glyph for -- and a score is not
     * written in them in any case.
     *
     * Held control types nothing at all, or a copy would leave a `c`
     * behind it.
     *
     * Characters rather than one character, because Tab indents by two.
     *
     * @param key The key that went down.
     * @param modifiers What was held with it.
     * @param layout Which board to read it off.
     * @return The characters, which outlive every caller, or an empty
     * view when the key types none.
     */
    [[nodiscard]] std::string_view typedTextFor(
        antwika::input::Key key,
        antwika::input::KeyModifiers modifiers,
        KeyLayout layout) noexcept;

    /**
     * @brief Get what a layout is called in the editor's own words.
     * @param layout The layout to name.
     * @return Its name, which outlives every caller.
     */
    [[nodiscard]] std::string_view nameOf(KeyLayout layout) noexcept;

} // namespace antwika::music_editor
