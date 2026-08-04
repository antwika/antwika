#pragma once

#include <optional>

#include <antwika/input/Key.hpp>
#include <antwika/ui/Keyboard.hpp>

#include <antwika/console/Typing.hpp>

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
     * @brief The typing tables moved to antwika::console with the
     * debug console; the name is kept here on InputFold.hpp's terms.
     */
    using antwika::console::typedCharacterFor;

} // namespace antwika::game
