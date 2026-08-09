#pragma once

#include <cstddef>

#include <antwika/event/ITickEventSink.hpp>
#include <antwika/event/TickEvent.hpp>

#include "antwika/console/ConsoleEvents.hpp"
#include "antwika/console/ConsoleState.hpp"
#include "antwika/console/InputFold.hpp"

namespace antwika::console
{

    using antwika::event::ITickEventSink;
    using antwika::event::TickEvent;

    class ConsoleGatedSink final : public ITickEventSink
    {
    public:
        ConsoleGatedSink(
            ITickEventSink &inner,
            const ConsoleState &console,
            const InputFold &input,
            ConsoleEvents &events);

        ConsoleGatedSink(const ConsoleGatedSink &) = delete;
        ConsoleGatedSink(ConsoleGatedSink &&) = delete;

        ConsoleGatedSink &operator=(const ConsoleGatedSink &) = delete;
        ConsoleGatedSink &operator=(ConsoleGatedSink &&) = delete;

        void handle(const TickEvent &event) override;

    private:
        void deliver(const TickEvent &event);

        ITickEventSink &inner;
        const ConsoleState &console;
        const InputFold &input;
        ConsoleEvents &events;
        std::size_t reader;
    };

}
