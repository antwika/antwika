#pragma once

#include <optional>
#include <vector>

#include <antwika/event/Event.hpp>
#include <antwika/event/ITickEventSource.hpp>
#include <antwika/time/Tick.hpp>

#include "antwika/companion/CompanionMemory.hpp"

namespace antwika::companion
{

    using antwika::event::Event;
    using antwika::event::ITickEventSource;

    /**
     * @brief Encode a remembered companion as this application's
     * companion.restore event.
     * @param memory The companion and the record behind it.
     * @return The event, payload and all.
     */
    [[nodiscard]] Event restoreEvent(const CompanionMemory &memory);

    /**
     * @brief Announces the companion a live session starts on, once.
     *
     * The companion comes from outside the program -- a `companion.json`
     * somebody's last session left -- so it is external input, and
     * external input reaches a simulation through the source the loop
     * pulls from. Putting it here rather than into a constructor is what
     * puts it *upstream of the recorder*, so a `--record` run writes the
     * companion it was played on into its own file and replaying that
     * file reaches the same animal rather than a brand new one.
     *
     * It is the only thing this application adds to the stream, and it
     * adds nothing at all on a replay: a recording already carries its
     * companion, and a second one would replace it halfway through the
     * session that was recorded on it.
     */
    class RestoreSource final : public ITickEventSource
    {
    public:
        /**
         * @brief Construct the decorator over what it wraps.
         * @param inner The source whose events pass through; must
         * outlive this object.
         * @param memory The companion to announce on the first tick, or
         * nothing for a session that starts new -- which is every
         * replay, and every live run with no store behind it.
         */
        RestoreSource(
            ITickEventSource &inner,
            std::optional<CompanionMemory> memory);

        RestoreSource(const RestoreSource &) = delete;
        RestoreSource(RestoreSource &&) = delete;

        RestoreSource &operator=(const RestoreSource &) = delete;
        RestoreSource &operator=(RestoreSource &&) = delete;

        /**
         * @brief Get a tick's events, the companion ahead of the first
         * tick's.
         *
         * Ahead of them rather than after, so a press on the very tick
         * the companion arrives on lands on the restored animal.
         * Announced on the first tick this is asked about rather than on
         * a tick named by number: a source is asked once per tick in
         * increasing order, so the first question is the first tick.
         *
         * @param tick The tick to fetch events for.
         * @return The wrapped source's events, preceded on the first
         * tick by one companion.restore.
         */
        [[nodiscard]] std::vector<Event> eventsFor(
            antwika::time::Tick tick) override;

    private:
        ITickEventSource &inner;
        std::optional<CompanionMemory> memory;
    };

} // namespace antwika::companion
