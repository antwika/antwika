#include "antwika/game/SaveLoadSink.hpp"

#include <optional>
#include <string_view>
#include <utility>
#include <variant>

#include <antwika/engine/Events.hpp>
#include <antwika/input/InputEvent.hpp>
#include <antwika/input/MouseButton.hpp>

#include "antwika/game/KeyText.hpp"
#include "antwika/game/SaveDirectory.hpp"
#include "antwika/game/SaveFormatError.hpp"
#include "antwika/game/SaveGameFile.hpp"

namespace antwika::game
{

    using antwika::input::InputEvent;
    using antwika::input::KeyPressed;
    using antwika::input::MouseButton;
    using antwika::input::PointerButtonPressed;

    namespace
    {
        [[nodiscard]] bool isLeftPress(const InputEvent &event) noexcept
        {
            const auto *pressed =
                std::get_if<PointerButtonPressed>(&event);

            return pressed != nullptr
                   && pressed->button == MouseButton::Left;
        }
    } // namespace

    SaveLoadSink::SaveLoadSink(
        SaveLoadState &state,
        AppModeState &mode,
        UiOverlay &overlay,
        const InputFold &input,
        const SaveLoadScene &scene,
        SessionStore &session,
        std::string directory)
        : state(state),
          mode(mode),
          overlay(overlay),
          input(input),
          scene(scene),
          session(session),
          directory(std::move(directory))
    {
    }

    void SaveLoadSink::handle(const TickEvent &event)
    {
        // Nothing at all in any other mode.
        // The screen is a mode of its own, as the menu is.
        if (mode.mode() != AppMode::SaveLoad)
        {
            return;
        }

        if (event.event.name == antwika::engine::events::kTick)
        {
            // Described again here, for the renderer about to paint.
            refreshAndAct(false, Keyboard{});
            return;
        }

        // Whatever the fold was just given, since it runs first.
        const auto &decoded = input.current();
        if (!decoded.has_value())
        {
            return;
        }

        // The characters live here for as long as the Context does.
        // ui::Keyboard borrows them rather than owning them.
        std::string characters;
        Keyboard keyboard;

        if (const auto *key = std::get_if<KeyPressed>(&*decoded))
        {
            const auto meaning = uiKeyFor(key->key, key->modifiers.shift);
            if (meaning.has_value())
            {
                keyboard.keys.push_back(*meaning);
            }

            const char typed =
                typedCharacterFor(key->key, key->modifiers.shift);
            if (typed != '\0')
            {
                // The edge is what says where in the order it lands.
                // A character with none is never typed at all.
                characters.push_back(typed);
                keyboard.keys.push_back(antwika::ui::Key::Character);
            }
        }

        keyboard.typed = characters;

        refreshAndAct(isLeftPress(*decoded), keyboard);
    }

    Pointer SaveLoadSink::pointerNow(bool pressed) const
    {
        const auto &mouse = input.state().mouse();

        return Pointer{
            .position = input.located()
                            ? std::optional<Point>{input.pointer()}
                            : std::nullopt,
            .down = mouse.isDown(MouseButton::Left),
            .pressed = pressed};
    }

    void SaveLoadSink::refreshAndAct(
        bool pressed, const Keyboard &keyboard)
    {
        auto frame = scene.describe(
            overlay.canvas(), pointerNow(pressed), keyboard, state);

        act(frame.interactions);

        // What was just typed, chosen or written is not in that picture.
        // So it is described once more, and the second one is drawn.
        // The same remedy ui::Context::finish() spells out.
        frame = scene.describe(
            overlay.canvas(), pointerNow(pressed), Keyboard{}, state);

        overlay.set(
            std::move(frame.commands),
            std::move(frame.hoverTargets),
            frame.interactions.pointerOverUi);
    }

    void SaveLoadSink::act(const Interactions &interactions)
    {
        // A press moves the keyboard onto what it hit.
        // antwika::ui will not, until focus is already in play.
        // That is deliberate: a pointer-only caller gains no ring.
        // This screen has a field, so it asks for one.
        // Pressing a box to type in it is what anybody means by it.
        state.setFocus(
            interactions.activated == antwika::ui::kNoWidget
                ? interactions.focused
                : interactions.activated);

        if (interactions.chosen.has_value())
        {
            state.select(interactions.chosen->index);
            state.setListOpen(false);
            return;
        }

        if (interactions.edit.has_value())
        {
            state.setName(
                interactions.edit->text, interactions.edit->cursor);

            // Enter in the field is its submit, so it saves.
            // The same keystroke also activates the field itself.
            // Which is why nothing below can match it.
            if (interactions.edit->submitted)
            {
                saveNow();
            }
            return;
        }

        const auto activated = interactions.activated;

        if (activated == saveWidgets::kPicker)
        {
            state.setListOpen(!state.listOpen());
        }
        else if (activated == saveWidgets::kSave)
        {
            saveNow();
        }
        else if (activated == saveWidgets::kLoad)
        {
            loadNow();
        }
        else if (activated == saveWidgets::kBack)
        {
            mode.request(AppMode::MainMenu);
        }
    }

    void SaveLoadSink::saveNow()
    {
        // What was typed wins over what is selected.
        // Typing a name is how a session is saved somewhere new.
        const std::string_view chosen =
            state.name().empty() ? state.selectedName() : state.name();

        if (chosen.empty())
        {
            state.setMessage("Name it first");
            return;
        }

        const std::string name(chosen);

        try
        {
            saveGameFile(session.take(), saveGamePath(directory, name));
        }
        // The excluded line's second branch is the catch's own.
        // It is taken by an exception this catch does not match.
        // Nothing under saveGameFile() throws anything else.
        catch (const SaveFormatError &failed) // GCOVR_EXCL_LINE
        {
            state.setMessage(
                std::string("Could not save: ") + failed.what());
            return;
        }

        // Added rather than re-listed.
        // A directory read inside the tick path would not replay.
        state.add(name);
        state.setName({}, 0);
        state.setMessage("Saved " + name);
    }

    void SaveLoadSink::loadNow()
    {
        const std::string_view selected = state.selectedName();

        if (selected.empty())
        {
            state.setMessage("Pick a save first");
            return;
        }

        const std::string name(selected);

        try
        {
            session.restore(
                loadGameFile(saveGamePath(directory, name)));
        }
        // Likewise, and for the same reason.
        catch (const SaveFormatError &failed) // GCOVR_EXCL_LINE
        {
            state.setMessage(
                std::string("Could not load: ") + failed.what());
            return;
        }

        state.setMessage("Loaded " + name);
        mode.request(AppMode::CityMap);
    }

} // namespace antwika::game
