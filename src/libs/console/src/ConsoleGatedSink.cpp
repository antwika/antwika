#include "antwika/console/ConsoleGatedSink.hpp"

#include <exception>
#include <string>
#include <utility>
#include <variant>

#include <antwika/input/InputEvent.hpp>

namespace antwika::console
{

    using antwika::input::KeyPressed;
    using antwika::input::KeyReleased;
    using antwika::input::PointerButtonPressed;
    using antwika::input::PointerScrolled;

    ConsoleGatedSink::ConsoleGatedSink(
        ITickEventSink &inner,
        const ConsoleState &console,
        const InputFold &input,
        ConsoleEvents &events)
        : inner(inner),
          console(console),
          input(input),
          events(events),
          reader(events.open())
    {
    }

    void ConsoleGatedSink::deliver(const TickEvent &event)
    {
        for (auto &sent : events.take(reader))
        {
            const auto named = sent.name; // GCOVR_EXCL_LINE

            try
            {
                inner.handle(
                    TickEvent{
                        .tick = event.tick, .event = std::move(sent)});
            }
            catch (const std::exception &refused) // GCOVR_EXCL_LINE
            {
                events.refuse(named, refused.what());
            }
        }
    }

    void ConsoleGatedSink::handle(const TickEvent &event)
    {
        deliver(event);

        const auto &decoded = input.current();

        if (!console.visible() || !decoded.has_value())
        {
            inner.handle(event);
            return;
        }

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

        if (std::holds_alternative<PointerScrolled>(*decoded)
            && console.covers(input.pointer()))
        {
            return;
        }

        inner.handle(event);
    }

}
