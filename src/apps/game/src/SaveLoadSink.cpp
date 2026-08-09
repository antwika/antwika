#include "antwika/game/SaveLoadSink.hpp"

#include <optional>
#include <string_view>
#include <utility>
#include <variant>

#include <antwika/app/PointerReading.hpp>
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

    using antwika::app::isLeftPress;

    namespace
    {
    }

    SaveLoadSink::SaveLoadSink(
        SaveLoadState &state,
        AppModeState &mode,
        UiOverlay &overlay,
        const InputFold &input,
        const SaveLoadScene &scene,
        SessionStore &session,
        const OptionsState &options,
        std::string directory)
        : state(state),
          mode(mode),
          overlay(overlay),
          input(input),
          scene(scene),
          session(session),
          options(options),
          directory(std::move(directory))
    {
    }

    void SaveLoadSink::handle(const TickEvent &event)
    {
        if (mode.mode() != AppMode::SaveLoad)
        {
            return;
        }

        if (event.event.name == antwika::engine::events::kTick)
        {
            refreshAndAct(false, Keyboard{});
            return;
        }

        const auto &decoded = input.current();
        if (!decoded.has_value())
        {
            return;
        }

        std::string characters;
        Keyboard keyboard;

        if (const auto *key = std::get_if<KeyPressed>(&*decoded))
        {
            const auto meaning = uiKeyFor(key->key, key->modifiers.shift);
            if (meaning.has_value())
            {
                keyboard.keys.push_back(*meaning);
            }

            const char typed = typedCharacterFor(
                key->key, key->modifiers.shift, options.keyboard());
            if (typed != '\0')
            {
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

        frame = scene.describe(
            overlay.canvas(), pointerNow(pressed), Keyboard{}, state);

        overlay.set(
            std::move(frame.commands),
            std::move(frame.hoverTargets),
            frame.interactions.pointerOverUi);
    }

    void SaveLoadSink::act(const Interactions &interactions)
    {
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
        catch (const SaveFormatError &failed) // GCOVR_EXCL_LINE
        {
            state.setMessage(
                std::string("Could not save: ") + failed.what());
            return;
        }

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
        catch (const SaveFormatError &failed) // GCOVR_EXCL_LINE
        {
            state.setMessage(
                std::string("Could not load: ") + failed.what());
            return;
        }

        state.setMessage("Loaded " + name);
        mode.request(AppMode::CityMap);
    }

}
