#pragma once

#include <vector>

#include <antwika/event/Event.hpp>
#include <antwika/event/ITickEventSource.hpp>
#include <antwika/time/Tick.hpp>

#include "antwika/input/IInputEventCodec.hpp"
#include "antwika/input/Key.hpp"

namespace antwika::input
{

    using antwika::event::Event;
    using antwika::event::ITickEventSource;

    /**
     * @brief Ends a run when a chosen key goes down, by appending
     * engine.stop.
     *
     * engine.stop is genuine external input, not something the engine
     * regenerates, so it has to come from the source or a replay will not
     * stop at the tick the live run did.
     *
     * A decorator rather than a flag on LiveInputSource, per the project's
     * preference for composition over modifying an already-tested class.
     * Put it outside LiveInputSource, so it sees the key events that source
     * produced.
     */
    class StopOnKeySource final : public ITickEventSource
    {
    public:
        /**
         * @brief Construct the decorator over what it wraps.
         * @param inner The source whose events pass through; must outlive
         * this object.
         * @param codec Used to recognise a key-down among the events.
         * @param key The key that ends the run.
         */
        StopOnKeySource(
            ITickEventSource &inner, const IInputEventCodec &codec, Key key);

        StopOnKeySource(const StopOnKeySource &) = delete;
        StopOnKeySource(StopOnKeySource &&) = delete;

        StopOnKeySource &operator=(const StopOnKeySource &) = delete;
        StopOnKeySource &operator=(StopOnKeySource &&) = delete;

        /**
         * @brief Get a tick's events, plus a stop if the key went down.
         *
         * A repeat does not stop the run: holding the key would otherwise
         * be indistinguishable from pressing it, and one press should mean
         * one stop.
         *
         * @param tick The tick to fetch events for.
         * @return The wrapped source's events, followed by engine.stop if
         * this tick carried a press of the chosen key.
         */
        [[nodiscard]] std::vector<Event> eventsFor(
            antwika::time::Tick tick) override;

    private:
        ITickEventSource &inner;
        const IInputEventCodec &codec;
        Key key;
    };

} // namespace antwika::input
