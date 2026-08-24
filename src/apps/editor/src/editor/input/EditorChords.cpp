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

    input::KeyModifiers Editor::getHeldModifiers() const noexcept
    {
        return inputState.getKeyboard().getModifiers();
    }

    bool Editor::matchesChord(
        const Action action, const input::Key key) const
    {
        return keyBench.matchesChord(action, key, getHeldModifiers());
    }

    bool Editor::matchesChordWithShift(
        const Action action, const input::Key key) const
    {
        return keyBench.matchesChordWithShift(
            action, key, getHeldModifiers());
    }

    void Editor::applyRunKey(const input::Key key, const bool down)
    {
        if (keyBench.matchesHeld(Action::Run, key, getHeldModifiers()))
        {
            play.game->setRunning(down);
        }
    }

    void Editor::applyWalkKey(const input::Key key, const bool down)
    {
        const auto matches = [this, key](const Action act)
        {
            return keyBench.matchesHeld(act, key, getHeldModifiers());
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
        if (!keyBench.panelShown)
        {
            return false;
        }

        if (pressedKey.repeat || isModifierKey(pressedKey.key))
        {
            return true;
        }

        if (keyBench.rebindingAction.has_value())
        {
            if (pressedKey.key == input::Key::Escape)
            {
                keyBench.rebindingAction.reset();

                return true;
            }

            auto keyBindings = keyBench.getBindings();

            keyBindings[*keyBench.rebindingAction] = Chord{
                .key = pressedKey.key,
                .ctrl = getHeldModifiers().control,
                .shift = getHeldModifiers().shift,
                .alt = getHeldModifiers().alt};
            keyBench.takeBindings(std::move(keyBindings));
            keyBench.rebindingAction.reset();
            saveChords(keyBench.getBindings(), getChordsPath());
            showStatus("bound", false, 120);

            return true;
        }

        if (pressedKey.key == input::Key::Escape)
        {
            keyBench.panelShown = false;
        }

        return true;
    }

    std::string Editor::getChordsPath() const
    {
        return io::getAssetPath(std::string("keys.json"));
    }

}
