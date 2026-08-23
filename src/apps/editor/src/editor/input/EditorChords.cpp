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

    input::KeyModifiers Editor::getHeldModifiers() const noexcept
    {
        return inputState.getKeyboard().getModifiers();
    }

    bool Editor::matchesChord(
        const Action action, const input::Key key) const
    {
        return actions.matches(getActionKey(action), key, getHeldModifiers());
    }

    bool Editor::matchesChordWithShift(
        const Action action, const input::Key key) const
    {
        return actions.matches(
            getShiftedAction(action), key, getHeldModifiers());
    }

    void Editor::applyRunKey(const input::Key key, const bool down)
    {
        if (actions.matches(getHeldAction(Action::Run), key, getHeldModifiers()))
        {
            play.game->setRunning(down);
        }
    }

    void Editor::applyWalkKey(const input::Key key, const bool down)
    {
        const auto matches = [this, key](const Action act)
        {
            return actions.matches(getHeldAction(act), key, getHeldModifiers());
        };

        if (matches(Action::WalkNorth))
        {
            play.game->wasdKeys().north = down;
        }

        if (matches(Action::WalkSouth))
        {
            play.game->wasdKeys().south = down;
        }

        if (matches(Action::WalkWest))
        {
            play.game->wasdKeys().west = down;
        }

        if (matches(Action::WalkEast))
        {
            play.game->wasdKeys().east = down;
        }

        if (matches(Action::WalkNorthAlt))
        {
            play.game->arrowKeys().north = down;
        }

        if (matches(Action::WalkSouthAlt))
        {
            play.game->arrowKeys().south = down;
        }

        if (matches(Action::WalkWestAlt))
        {
            play.game->arrowKeys().west = down;
        }

        if (matches(Action::WalkEastAlt))
        {
            play.game->arrowKeys().east = down;
        }
    }

    bool Editor::consumeBindingsKey(const input::KeyPressed &pressedKey)
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
                .ctrl = getHeldModifiers().control,
                .shift = getHeldModifiers().shift,
                .alt = getHeldModifiers().alt};
            setBindings(std::move(keyBindings));
            rebindingAction.reset();
            saveChords(bindings, getChordsPath());
            showStatus("bound", false, 120);

            return true;
        }

        if (pressedKey.key == input::Key::Escape)
        {
            keysOpen = false;
        }

        return true;
    }

    std::string Editor::getChordsPath() const
    {
        return io::getAssetPath(std::string("keys.json"));
    }

}
