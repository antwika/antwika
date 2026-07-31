#pragma once

#include <optional>

#include <antwika/input/Key.hpp>
#include <antwika/ui/Keyboard.hpp>

namespace antwika::ui_demo
{

    /**
     * @brief Translate a key edge into the one antwika::ui acts on.
     *
     * The application's half of the seam antwika::ui deliberately leaves
     * open: the library names the keys it has a meaning for and reads no
     * device, so somebody has to say that this app's Tab is its
     * FocusNext.
     * It is here rather than in either library because which key does
     * what is an application's decision.
     *
     * Unlike apps/game's version, Escape does arrive as
     * ui::Key::Cancel: nothing stops this demo on Escape, so a field can
     * still be given up on -- which is the whole of what a showcase of
     * ui::TextEdit::cancelled needs.
     *
     * @param key The key that went down.
     * @param shift Whether shift was held, which is what tells Tab from
     * Shift+Tab. The library has no modifiers, since everything crossing
     * that seam is an edge and a modifier is a held state.
     * @return What the UI should be told, or nothing for a key it has no
     * meaning for.
     */
    [[nodiscard]] std::optional<antwika::ui::Key> uiKeyFor(
        antwika::input::Key key, bool shift) noexcept;

    /**
     * @brief Get the character a key types.
     *
     * A layout written down rather than asked of a window system, so
     * that what a recording holds is the symbolic key and the character
     * comes back identically under any backend on any keyboard.
     * That is the same reason InputEventCodec persists key names rather
     * than scancodes.
     *
     * Letters, digits, space, hyphen and full stop, which is as much as
     * a demo needs anybody to be able to type: a key with no character
     * here types nothing.
     *
     * @param key The key that went down.
     * @param shift Whether shift was held.
     * @return The character, or '\0' when the key types none.
     */
    [[nodiscard]] char typedCharacterFor(
        antwika::input::Key key, bool shift) noexcept;

} // namespace antwika::ui_demo
