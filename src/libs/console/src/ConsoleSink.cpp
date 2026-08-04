#include "antwika/console/ConsoleSink.hpp"

#include <optional>
#include <utility>
#include <variant>

#include <antwika/engine/Events.hpp>
#include <antwika/input/InputEvent.hpp>
#include <antwika/input/MouseButton.hpp>

#include "antwika/console/Typing.hpp"

namespace antwika::console
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

    ConsoleSink::ConsoleSink(const ConsoleSinkSetup &setup) : setup(setup)
    {
    }

    void ConsoleSink::handle(const TickEvent &event)
    {
        if (event.event.name == antwika::engine::events::kTick)
        {
            // The slide is ticks, so it moves here and only here.
            setup.console.advance();
            setup.console.setHeight(consoleHeightAt(
                setup.console.steps(), setup.picture.canvas()));

            // Described again here, for the renderer about to paint.
            refreshAndAct(false, Keyboard{});
            return;
        }

        // Whatever the fold was just given, since it runs first.
        const auto &decoded = setup.input.current();
        if (!decoded.has_value())
        {
            return;
        }

        const auto *key = std::get_if<KeyPressed>(&*decoded);

        // The toggle answers whether or not the console is open.
        // It is how the console closes, so it cannot be the field's.
        if (key != nullptr && !key->repeat
            && key->key == setup.controls.toggleKey())
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
                && key->key == setup.controls.executeKey())
            {
                // The bound key is what submits, whichever it is.
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
                    setup.controls.keyboard());
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
            setup.picture.canvas(),
            pointerNow(pressed),
            keyboard,
            setup.console);

        act(frame.interactions);

        // What was just typed or executed is not in that picture.
        // So it is described once more, and the second one is drawn.
        // The same remedy game::SaveLoadSink spells out.
        frame = setup.scene.describe(
            setup.picture.canvas(),
            pointerNow(pressed),
            Keyboard{},
            setup.console);

        setup.picture.set(std::move(frame.commands));
    }

    void ConsoleSink::act(const Interactions &interactions)
    {
        if (!interactions.edit.has_value())
        {
            return;
        }

        setup.console.setLine(
            interactions.edit->text, interactions.edit->cursor);

        // The execute key in the field is its submit, so it executes.
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

        setup.commands.execute(command, setup.console);
    }

} // namespace antwika::console
