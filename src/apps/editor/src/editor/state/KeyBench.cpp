#include "antwika/editor/editor/state/KeyBench.hpp"

#include <utility>

namespace antwika::editor
{

    void KeyBench::takeBindings(KeyBindings keyBindings)
    {
        bindings = std::move(keyBindings);
        actions = actionMapFrom(bindings);
    }

    const KeyBindings &KeyBench::getBindings() const noexcept
    {
        return bindings;
    }

    bool KeyBench::matchesChord(
        const Action action,
        const input::Key key,
        const input::KeyModifiers heldModifiers) const
    {
        return actions.matches(getActionKey(action), key, heldModifiers);
    }

    bool KeyBench::matchesChordWithShift(
        const Action action,
        const input::Key key,
        const input::KeyModifiers heldModifiers) const
    {
        return actions.matches(getShiftedAction(action), key, heldModifiers);
    }

    bool KeyBench::matchesHeld(
        const Action action,
        const input::Key key,
        const input::KeyModifiers heldModifiers) const
    {
        return actions.matches(getHeldAction(action), key, heldModifiers);
    }

}
