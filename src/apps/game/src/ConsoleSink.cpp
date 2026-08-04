#include "antwika/game/ConsoleSink.hpp"

#include <optional>
#include <utility>
#include <variant>

#include <antwika/engine/Events.hpp>
#include <antwika/input/InputEvent.hpp>
#include <antwika/input/MouseButton.hpp>

#include "antwika/game/Action.hpp"
#include "antwika/game/KeyText.hpp"
#include "antwika/game/SaveFormatError.hpp"
#include "antwika/game/StateDump.hpp"
#include "antwika/game/StateDumpFile.hpp"

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

        [[nodiscard]] std::string trimmed(const std::string &line)
        {
            const auto first = line.find_first_not_of(' ');

            if (first == std::string::npos)
            {
                return {};
            }

            const auto last = line.find_last_not_of(' ');

            return line.substr(first, last - first + 1);
        }
    } // namespace

    bool consoleLoadPermitted(bool recording, bool replaying) noexcept
    {
        return !recording && !replaying;
    }

    ConsoleSink::ConsoleSink(
        const ConsoleSinkSetup &setup, std::string dumpPath)
        : setup(setup), dumpPath(std::move(dumpPath))
    {
    }

    void ConsoleSink::handle(const TickEvent &event)
    {
        if (event.event.name == antwika::engine::events::kTick)
        {
            // Leaving the city takes the console with it.
            // Left open elsewhere, its gates would swallow keys.
            // And the toggle could never be reached to stop them.
            if (setup.mode.mode() != AppMode::CityMap)
            {
                setup.console.close();
            }

            // The slide is ticks, so it moves here and only here.
            setup.console.advance();
            setup.console.setHeight(consoleHeightAt(
                setup.console.steps(), setup.overlay.canvas()));

            // Described again here, for the renderer about to paint.
            refreshAndAct(false, Keyboard{});
            return;
        }

        // Nothing at all in any other mode.
        // The console is the city's -- see the tick arm above.
        if (setup.mode.mode() != AppMode::CityMap)
        {
            return;
        }

        // Whatever the fold was just given, since it runs first.
        const auto &decoded = setup.input.current();
        if (!decoded.has_value())
        {
            return;
        }

        const auto &bindings = setup.options.bindings();
        const auto *key = std::get_if<KeyPressed>(&*decoded);

        // The toggle answers whether or not the console is open.
        // It is how the console closes, so it cannot be the field's.
        if (key != nullptr && !key->repeat
            && key->key == bindings.keyFor(Action::ConsoleToggle))
        {
            setup.console.toggle();
            refreshAndAct(false, Keyboard{});
            return;
        }

        // Part way along the slide the field does not read.
        // That is the rule the whole animation state exists to keep.
        if (!setup.console.acceptsText())
        {
            return;
        }

        // The characters live here for as long as the Context does.
        // ui::Keyboard borrows them rather than owning them.
        std::string characters;
        Keyboard keyboard;

        if (key != nullptr)
        {
            if (!key->repeat
                && key->key == bindings.keyFor(Action::ConsoleExecute))
            {
                // The bound key is what submits, whichever it is.
                keyboard.keys.push_back(antwika::ui::Key::Activate);
            }
            else
            {
                const auto meaning =
                    uiKeyFor(key->key, key->modifiers.shift);

                // Activate stays the execute binding's alone.
                // Rebound away from Enter, Enter must stop executing.
                if (meaning.has_value()
                    && *meaning != antwika::ui::Key::Activate)
                {
                    keyboard.keys.push_back(*meaning);
                }

                const char typed =
                    typedCharacterFor(key->key, key->modifiers.shift);
                if (typed != '\0')
                {
                    // The edge says where in the order it lands.
                    // A character with none is never typed at all.
                    characters.push_back(typed);
                    keyboard.keys.push_back(
                        antwika::ui::Key::Character);
                }
            }
        }

        keyboard.typed = characters;

        refreshAndAct(isLeftPress(*decoded), keyboard);
    }

    Pointer ConsoleSink::pointerNow(bool pressed) const
    {
        const auto &mouse = setup.input.state().mouse();

        return Pointer{
            .position = setup.input.located()
                            ? std::optional<Point>{setup.input.pointer()}
                            : std::nullopt,
            .down = mouse.isDown(MouseButton::Left),
            .pressed = pressed};
    }

    void ConsoleSink::refreshAndAct(
        bool pressed, const Keyboard &keyboard)
    {
        auto frame = setup.scene.describe(
            setup.overlay.canvas(),
            pointerNow(pressed),
            keyboard,
            setup.console);

        act(frame.interactions);

        // What was just typed or executed is not in that picture.
        // So it is described once more, and the second one is drawn.
        // The same remedy SaveLoadSink spells out.
        frame = setup.scene.describe(
            setup.overlay.canvas(),
            pointerNow(pressed),
            Keyboard{},
            setup.console);

        setup.overlay.set(
            std::move(frame.commands),
            std::move(frame.hoverTargets),
            frame.interactions.pointerOverUi);
    }

    void ConsoleSink::act(const Interactions &interactions)
    {
        if (!interactions.edit.has_value())
        {
            return;
        }

        setup.console.setLine(
            interactions.edit->text, interactions.edit->cursor);

        // Enter in the field is its submit, so it executes.
        if (interactions.edit->submitted)
        {
            execute(trimmed(setup.console.takeLine()));
        }
    }

    void ConsoleSink::execute(const std::string &command)
    {
        // An empty line asks for nothing, so it is not history either.
        if (command.empty())
        {
            return;
        }

        setup.console.pushHistory("> " + command);

        if (command == "dump_state")
        {
            dumpState();
        }
        else if (command == "load_state")
        {
            loadState();
        }
        else
        {
            setup.console.pushHistory("unknown command: " + command);
        }
    }

    void ConsoleSink::dumpState()
    {
        // Answered before the state is taken, deliberately.
        // The dump then carries the whole exchange that made it.
        setup.console.pushHistory("dumped state to " + dumpPath);

        StateDump dump;
        dump.save = setup.session.take();
        dump.paused = setup.pause.paused();
        dump.tool = setup.toolbar.tool();
        dump.view = setup.view.view();
        dump.locale = setup.locale.locale();
        dump.console = setup.console.history();

        stateDumpFile(dump, dumpPath);

        // The excluded line is the local dump's unwind destructor.
        // Nothing after its construction throws but the write itself.
    } // GCOVR_EXCL_LINE

    void ConsoleSink::loadState()
    {
        // The console-level twin of requireRecordableStart().
        // A load reads a file no recording carries.
        // So a recorded or replayed run refuses it outright.
        // The refusal is a history line, and so deterministic.
        // A replay typing load_state reads what the live run read.
        if (!setup.loadEnabled)
        {
            setup.console.pushHistory(
                "load_state: not available while recording or "
                "replaying");
            return;
        }

        std::optional<StateDump> loaded;

        try
        {
            loaded = loadStateDump(dumpPath);
        }
        // The excluded line's second branch is the catch's own.
        // It is taken by an exception this catch does not match.
        // Nothing under loadStateDump() throws anything else.
        catch (const SaveFormatError &failed) // GCOVR_EXCL_LINE
        {
            setup.console.pushHistory(
                std::string("could not load: ") + failed.what());
            return;
        }

        setup.session.restore(loaded->save);
        setup.pause.set(loaded->paused);

        if (loaded->tool.has_value())
        {
            setup.toolbar.select(*loaded->tool);
        }
        else
        {
            setup.toolbar.clearTool();
        }

        setup.view.set(loaded->view);

        // Staged rather than switched.
        // So it lands at the tick boundary, as the options screen's.
        setup.locale.request(loaded->locale);

        setup.console.replaceHistory(loaded->console);
        setup.console.pushHistory(
            "loaded state from " + dumpPath);
    }

} // namespace antwika::game
