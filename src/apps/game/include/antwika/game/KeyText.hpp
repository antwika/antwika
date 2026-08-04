#pragma once

#include <optional>

#include <antwika/input/Key.hpp>
#include <antwika/ui/Keyboard.hpp>

#include "antwika/game/KeyboardLayout.hpp"

namespace antwika::game
{

    /**
     * @brief Translate a key edge into the one antwika::ui acts on.
     *
     * The application's half of the seam antwika::ui deliberately leaves
     * open: the library names the keys it has a meaning for and reads no
     * device, so somebody has to say that this app's Tab is its
     * FocusNext. It is here rather than in either library because which
     * key does what is an application's decision.
     *
     * ui::Key::Cancel is deliberately never produced. Escape ends a run
     * in this app -- InputPipeline stops on it, upstream of every sink --
     * so a field could not read it even if this mapped it, and a branch
     * nothing can reach is one the coverage gate would demand an
     * impossible test for.
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
     * @brief Get the character a key types, on one layout.
     *
     * A layout written down rather than asked of a window system, so
     * that what a recording holds is the symbolic key and the character
     * comes back identically under any backend on any keyboard. That is
     * the same reason InputEventCodec persists key names rather than
     * scancodes -- and which written-down layout a run types by is
     * itself simulation state, announced onto the wire, so a session
     * typed on one machine replays its characters on any other.
     *
     * ASCII only, deliberately: letters, digits, space and the
     * punctuation a command or a file name needs.
     * The Swedish letters would be two bytes each in the UTF-8 the UI
     * holds, and a text field's caret and click arithmetic count
     * bytes, so those three keys type nothing rather than something
     * a caret would land inside.
     *
     * @param key The key that went down.
     * @param shift Whether shift was held.
     * @param layout Which board decides what the position types.
     * @return The character, or '\0' when the key types none.
     */
    [[nodiscard]] char typedCharacterFor(
        antwika::input::Key key,
        bool shift,
        KeyboardLayout layout) noexcept;

} // namespace antwika::game
