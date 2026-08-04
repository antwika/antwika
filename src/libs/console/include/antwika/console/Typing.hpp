#pragma once

#include <optional>

#include <antwika/input/Key.hpp>
#include <antwika/ui/Keyboard.hpp>

#include "antwika/console/KeyboardLayout.hpp"

namespace antwika::console
{

    /**
     * @brief Get the character a key types, on one layout.
     *
     * A layout written down rather than asked of a window system, so
     * that what a recording holds is the symbolic key and the character
     * comes back identically under any backend on any keyboard. That is
     * the same reason input::InputEventCodec persists key names rather
     * than scancodes -- and which written-down layout a run types by is
     * an application's own simulation state, announced onto the wire by
     * whatever machinery that application has for its options.
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

    /**
     * @brief Translate a key edge into the one the console's field acts
     * on.
     *
     * The console's own half of the seam antwika::ui leaves open, and
     * deliberately smaller than an application's: one field means no
     * focus keys, and executing is the console's own bound key rather
     * than ui::Key::Activate here -- see ConsoleSink.
     *
     * @param key The key that went down.
     * @return What the field should be told, or nothing for a key it
     * has no meaning for.
     */
    [[nodiscard]] std::optional<antwika::ui::Key> consoleKeyFor(
        antwika::input::Key key) noexcept;

} // namespace antwika::console
