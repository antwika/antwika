#pragma once

#include <optional>
#include <string>
#include <vector>

#include <antwika/input/ActionMap.hpp>
#include <antwika/input/Key.hpp>
#include <antwika/input/KeyModifiers.hpp>
#include <antwika/ui/Keyboard.hpp>

#include "antwika/editor/ui/EditorBindings.hpp"

namespace antwika::editor
{

    class KeyBench final
    {
    public:
        void takeBindings(KeyBindings keyBindings);

        [[nodiscard]] const KeyBindings &getBindings() const noexcept;

        [[nodiscard]] bool matchesChord(
            Action action,
            input::Key key,
            input::KeyModifiers heldModifiers) const;

        [[nodiscard]] bool matchesChordWithShift(
            Action action,
            input::Key key,
            input::KeyModifiers heldModifiers) const;

        [[nodiscard]] bool matchesHeld(
            Action action,
            input::Key key,
            input::KeyModifiers heldModifiers) const;

        bool panelShown = false;

        std::optional<Action> rebindingAction;

        std::string typedThisFrame;

        std::vector<ui::Key> keysNow;

    private:
        KeyBindings bindings = getDefaultChords();

        input::ActionMap actions = actionMapFrom(bindings);
    };

}
