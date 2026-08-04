#pragma once

#include <antwika/event/ITickEventSink.hpp>
#include <antwika/event/TickEvent.hpp>

#include "antwika/console/ConsoleState.hpp"
#include "antwika/console/InputFold.hpp"

namespace antwika::console
{

    using antwika::event::ITickEventSink;
    using antwika::event::TickEvent;

    /**
     * @brief Keeps input the console stands over off a wrapped sink.
     *
     * The console is on top, so what it stands over it takes: while
     * any of it is out, every key edge is the console's, and a press
     * or a scroll above its bottom edge is too.
     * Movements and releases still pass, on the toolbar's own terms
     * -- a pan begun on the city carries on across the console, and a
     * drag let go over it is still let go.
     *
     * A decorator rather than a flag inside each sink, for
     * ModeGatedSink's reason exactly: "the console covers this" is
     * stated once, where the sink is registered, and a sink under it
     * needs no opinion about consoles at all.
     *
     * Everything it reads is simulation state folded from recorded
     * input, so a replay gates identically by construction.
     */
    class ConsoleGatedSink final : public ITickEventSink
    {
    public:
        /**
         * @brief Construct the gate around one sink.
         * @param inner The sink to forward to. Must outlive this one.
         * @param console The console whose reach decides. Must outlive
         * this sink.
         * @param input The folded input, registered ahead of this
         * sink. Must outlive this sink.
         */
        ConsoleGatedSink(
            ITickEventSink &inner,
            const ConsoleState &console,
            const InputFold &input) noexcept;

        ConsoleGatedSink(const ConsoleGatedSink &) = delete;
        ConsoleGatedSink(ConsoleGatedSink &&) = delete;

        ConsoleGatedSink &operator=(const ConsoleGatedSink &) = delete;
        ConsoleGatedSink &operator=(ConsoleGatedSink &&) = delete;

        /**
         * @brief Forward a tick event, unless the console claims it.
         * @param event Forwarded whenever it is not an input.* event
         * the console stands over; engine.tick always reaches the
         * inner sink, since a gate changes what a click means and
         * never whether a picture is described.
         */
        void handle(const TickEvent &event) override;

    private:
        ITickEventSink &inner;
        const ConsoleState &console;
        const InputFold &input;
    };

} // namespace antwika::console
