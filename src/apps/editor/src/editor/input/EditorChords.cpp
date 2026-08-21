#include <utility>

#include <antwika/io/AssetPath.hpp>
#include <antwika/input/InputEvent.hpp>
#include <antwika/input/Key.hpp>

#include "antwika/editor/Editor.hpp"

namespace
{

    [[nodiscard]] bool isModifierKey(const antwika::input::Key key)
    {
        switch (key)
        {
        case antwika::input::Key::LeftShift:
        case antwika::input::Key::RightShift:
        case antwika::input::Key::LeftControl:
        case antwika::input::Key::RightControl:
        case antwika::input::Key::LeftAlt:
        case antwika::input::Key::RightAlt:
        case antwika::input::Key::LeftSuper:
        case antwika::input::Key::RightSuper:
        case antwika::input::Key::CapsLock:
            return true;
        default:
            break;
        }

        return false;
    }

}

namespace antwika::editor
{

    void Editor::setBindings(KeyBindings keyBindings)
    {
        bindings = std::move(keyBindings);
        actions = actionMapFrom(bindings);
    }

    input::KeyModifiers Editor::heldModifiers() const noexcept
    {
        return inputState.keyboard().modifiers();
    }

    bool Editor::matchesChord(
        const Action action, const input::Key key) const
    {
        return actions.matches(actionKey(action), key, heldModifiers());
    }

    bool Editor::matchesChordWithShift(
        const Action action, const input::Key key) const
    {
        return actions.matches(
            shiftedAction(action), key, heldModifiers());
    }

    void Editor::applyRunKey(const input::Key key, const bool down)
    {
        if (actions.matches(heldAction(Action::Run), key, heldModifiers()))
        {
            game->setRunning(down);
        }
    }

    void Editor::applyWalkKey(const input::Key key, const bool down)
    {
        const auto matches = [this, key](const Action act)
        {
            return actions.matches(heldAction(act), key, heldModifiers());
        };

        if (matches(Action::WalkNorth))
        {
            game->wasdKeys().north = down;
        }

        if (matches(Action::WalkSouth))
        {
            game->wasdKeys().south = down;
        }

        if (matches(Action::WalkWest))
        {
            game->wasdKeys().west = down;
        }

        if (matches(Action::WalkEast))
        {
            game->wasdKeys().east = down;
        }

        if (matches(Action::WalkNorthAlt))
        {
            game->arrowKeys().north = down;
        }

        if (matches(Action::WalkSouthAlt))
        {
            game->arrowKeys().south = down;
        }

        if (matches(Action::WalkWestAlt))
        {
            game->arrowKeys().west = down;
        }

        if (matches(Action::WalkEastAlt))
        {
            game->arrowKeys().east = down;
        }
    }

    bool Editor::handleBindingsKey(const input::KeyPressed &pressedKey)
    {
        if (!keysOpen)
        {
            return false;
        }

        if (pressedKey.repeat || isModifierKey(pressedKey.key))
        {
            return true;
        }

        if (rebindingAction.has_value())
        {
            if (pressedKey.key == input::Key::Escape)
            {
                rebindingAction.reset();

                return true;
            }

            auto keyBindings = bindings;

            keyBindings[*rebindingAction] = Chord{
                .key = pressedKey.key,
                .ctrl = heldModifiers().control,
                .shift = heldModifiers().shift,
                .alt = heldModifiers().alt};
            setBindings(std::move(keyBindings));
            rebindingAction.reset();
            saveChords(bindings, chordsPath());
            showStatus("bound", false, 120);

            return true;
        }

        if (pressedKey.key == input::Key::Escape)
        {
            keysOpen = false;
        }

        return true;
    }

    std::string Editor::chordsPath() const
    {
        return io::assetPath(std::string("keys.json"));
    }

}
