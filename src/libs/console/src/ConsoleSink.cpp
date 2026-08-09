#include "antwika/console/ConsoleSink.hpp"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

#include <antwika/engine/Events.hpp>
#include <antwika/event/Event.hpp>
#include <antwika/input/InputEvent.hpp>
#include <antwika/input/Key.hpp>
#include <antwika/input/MouseButton.hpp>

#include "antwika/console/Typing.hpp"

namespace antwika::console
{

    using antwika::engine::events::kStop;
    using antwika::event::Event;
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

        struct Split final
        {
            std::string word;

            std::string rest;
        };

        [[nodiscard]] Split splitFirst(const std::string &line)
        {
            const auto space = line.find(' ');

            Split asked; // GCOVR_EXCL_LINE
            asked.word = line.substr(0, space); // GCOVR_EXCL_LINE

            if (space != std::string::npos)
            {
                asked.rest = // GCOVR_EXCL_LINE
                    trimmed(line.substr(space + 1));
            }

            return asked;
        } // GCOVR_EXCL_LINE

        constexpr std::string_view kSendCommand = "send";

        constexpr std::string_view kQuitCommand = "quit";

        constexpr std::string_view kCommandCommand = "command";

        constexpr std::string_view kListWord = "list";

        [[nodiscard]] bool recalls(
            const antwika::input::Key key) noexcept
        {
            return key == antwika::input::Key::ArrowUp
                   || key == antwika::input::Key::ArrowDown;
        }
    }

    ConsoleSink::ConsoleSink(const ConsoleSinkSetup &setup) : setup(setup)
    {
    }

    void ConsoleSink::handle(const TickEvent &event)
    {
        reportRefusals();

        if (event.event.name == antwika::engine::events::kTick)
        {
            setup.console.advance();
            setup.console.setHeight(consoleHeightAt(
                setup.console.steps(), setup.picture.canvas()));

            refreshAndAct(event.tick, false, Keyboard{});
            return;
        }

        const auto &decoded = setup.input.current();
        if (!decoded.has_value())
        {
            return;
        }

        const auto *key = std::get_if<KeyPressed>(&*decoded);

        if (key != nullptr && !key->repeat
            && key->key == setup.controls.toggleKey())
        {
            setup.console.toggle();
            refreshAndAct(event.tick, false, Keyboard{});
            return;
        }

        if (!setup.console.acceptsText())
        {
            return;
        }

        if (key != nullptr && recalls(key->key))
        {
            setup.console.recall(
                key->key == antwika::input::Key::ArrowUp);

            refreshAndAct(event.tick, false, Keyboard{});
            return;
        }

        std::string characters;
        Keyboard keyboard;

        if (key != nullptr)
        {
            if (!key->repeat
                && key->key == setup.controls.executeKey())
            {
                keyboard.keys.push_back(antwika::ui::Key::Activate);
            }
            else
            {
                const auto meaning = consoleKeyFor(key->key);

                if (meaning.has_value())
                {
                    keyboard.keys.push_back(*meaning);
                }

                const char typed = typedCharacterFor(
                    key->key,
                    key->modifiers.shift,
                    setup.controls.keyboard(),
                    key->modifiers.alt);
                if (typed != '\0')
                {
                    characters.push_back(typed);
                    keyboard.keys.push_back(
                        antwika::ui::Key::Character);
                }
            }
        }

        keyboard.typed = characters;

        refreshAndAct(event.tick, isLeftPress(*decoded), keyboard);
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
        antwika::time::Tick tick,
        bool pressed,
        const Keyboard &keyboard)
    {
        auto frame = setup.scene.describe(
            setup.picture.canvas(),
            pointerNow(pressed),
            keyboard,
            setup.console);

        act(tick, frame.interactions);

        frame = setup.scene.describe(
            setup.picture.canvas(),
            pointerNow(pressed),
            Keyboard{},
            setup.console);

        setup.picture.set(std::move(frame.commands));
    }

    void ConsoleSink::act(
        antwika::time::Tick tick, const Interactions &interactions)
    {
        if (!interactions.edit.has_value())
        {
            return;
        }

        setup.console.setLine(
            interactions.edit->text, interactions.edit->cursor);

        if (interactions.edit->submitted)
        {
            execute(tick, trimmed(setup.console.takeLine()));
        }
    }

    void ConsoleSink::execute(
        antwika::time::Tick tick, const std::string &command)
    {
        if (command.empty())
        {
            return;
        }

        setup.console.pushHistory("> " + command);
        setup.console.rememberCommand(command);

        if (command == kQuitCommand && setup.stop.has_value())
        {
            setup.console.pushHistory("quitting");

            const Event stopping{.name = kStop}; // GCOVR_EXCL_LINE

            setup.stop->get().handle(
                TickEvent{.tick = tick, .event = stopping});
            return;
        }

        const auto asked = splitFirst(command);

        if (asked.word == kSendCommand)
        {
            send(asked.rest);
            return;
        }

        if (asked.word == kCommandCommand)
        {
            listCommands(asked.rest);
            return;
        }

        setup.commands.execute(command, setup.console);
    }

    void ConsoleSink::listCommands(const std::string &arguments)
    {
        if (arguments != kListWord)
        {
            setup.console.pushHistory(
                "command: say " + std::string{kListWord});
            return;
        }

        std::vector<std::string> named;

        named.emplace_back(
            std::string{kCommandCommand} + ' ' + std::string{kListWord});

        if (setup.stop.has_value())
        {
            named.emplace_back(kQuitCommand);
        }

        if (setup.events.has_value())
        {
            named.emplace_back(kSendCommand);
        }

        for (auto &name : setup.commands.names())
        {
            named.push_back(std::move(name));
        }

        std::ranges::sort(named);

        for (const auto &name : named)
        {
            setup.console.pushHistory(name);
        }
    }

    void ConsoleSink::reportRefusals()
    {
        if (!setup.events.has_value())
        {
            return;
        }

        for (const auto &refusal : setup.events->get().takeRefusals())
        {
            setup.console.pushHistory(refusal);
        }
    }

    void ConsoleSink::send(const std::string &arguments)
    {
        if (!setup.events.has_value())
        {
            setup.console.pushHistory(
                "send: this run carries no event queue");
            return;
        }

        const auto asked = splitFirst(arguments);

        if (asked.word.empty())
        {
            setup.console.pushHistory("send: name the event to send");
            return;
        }

        if (!asked.rest.empty() && !nlohmann::json::accept(asked.rest))
        {
            setup.console.pushHistory("send: the payload is not json");
            return;
        }

        setup.events->get().send(Event{ // GCOVR_EXCL_LINE
            .name = asked.word,
            .payload = asked.rest}); // GCOVR_EXCL_LINE

        setup.console.pushHistory("sent " + asked.word);
    }

}
