#include "antwika/game/ConsoleGatedSink.hpp"

#include <variant>

#include <antwika/input/InputEvent.hpp>

namespace antwika::game
{

    using antwika::input::KeyPressed;
    using antwika::input::KeyReleased;
    using antwika::input::PointerButtonPressed;
    using antwika::input::PointerScrolled;

    ConsoleGatedSink::ConsoleGatedSink(
        ITickEventSink &inner,
        const ConsoleState &console,
        const InputFold &input) noexcept
        : inner(inner), console(console), input(input)
    {
    }

    void ConsoleGatedSink::handle(const TickEvent &event)
    {
        // Whatever the fold was just given, since it runs first.
        // Nothing decoded means nothing the console could claim.
        const auto &decoded = input.current();

        if (!console.visible() || !decoded.has_value())
        {
            inner.handle(event);
            return;
        }

        // Every key edge is the console's while any of it is out.
        // Typing a command must not also zoom the city.
        if (std::holds_alternative<KeyPressed>(*decoded)
            || std::holds_alternative<KeyReleased>(*decoded))
        {
            return;
        }

        if (const auto *press =
                std::get_if<PointerButtonPressed>(&*decoded))
        {
            if (console.covers(Point{
                    .x = press->position.x, .y = press->position.y}))
            {
                return;
            }
        }

        // A scroll carries no position of its own.
        // The fold's is where the wheel is -- see IdleMotionSource.
        if (std::holds_alternative<PointerScrolled>(*decoded)
            && console.covers(input.pointer()))
        {
            return;
        }

        inner.handle(event);
    }

} // namespace antwika::game
