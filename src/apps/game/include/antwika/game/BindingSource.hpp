#pragma once

#include <optional>
#include <vector>

#include <antwika/event/Event.hpp>
#include <antwika/simulation/ITickEventSource.hpp>
#include <antwika/time/Tick.hpp>

#include "antwika/game/KeyBindings.hpp"

namespace antwika::game
{

    using antwika::event::Event;
    using antwika::simulation::ITickEventSource;

    /**
     * @brief Puts the machine's own key bindings on the wire, once, at
     * the start of a live run.
     *
     * **This is the seam that keeps a rebindable key from breaking a
     * replay**, and it is the same shape as
     * input::InputPipelineOptions::readsDevice: a live run reads the
     * device and a replay reads the file, through one class, with the
     * difference stated once at construction.
     *
     * A key binding read off the player's options file is externally
     * supplied and is not derivable from anything else in the run, so
     * the cross-module rule that only externally-supplied input is
     * persisted says outright that it has to be recorded. Announcing it
     * here, upstream of event::TickEventRecorder, is what records it:
     * the recorder writes unconditionally, so a --record file carries
     * the layout the session was played under and a replay resolves the
     * recorded key presses against that rather than against whatever the
     * replaying machine happens to be bound to.
     *
     * **A replay announces nothing**, because the recording already
     * holds the announcement. Constructed with no bindings at all -- see
     * MachineOptions -- it is a pure pass-through, and the events it
     * hands on are the file's, unchanged.
     *
     * **Only a binding that differs from kDefaultBindings is announced.**
     * A run on a machine that has never opened the options screen
     * therefore records exactly what it recorded before this class
     * existed, and every replay written before it still means what it
     * meant.
     *
     * The announcement goes ahead of the tick's own events rather than
     * after them, so the first click of a session is already resolved
     * against the layout the session was played under.
     */
    class BindingSource final : public ITickEventSource
    {
    public:
        /**
         * @brief Construct the announcer over what it wraps.
         * @param inner The source whose events pass through; must
         * outlive this object.
         * @param announced The layout the machine is carrying, or
         * nothing for a run whose input comes from a file and which
         * therefore already holds one.
         */
        BindingSource(
            ITickEventSource &inner,
            std::optional<KeyBindings> announced) noexcept;

        BindingSource(const BindingSource &) = delete;
        BindingSource(BindingSource &&) = delete;

        BindingSource &operator=(const BindingSource &) = delete;
        BindingSource &operator=(BindingSource &&) = delete;

        /**
         * @brief Get a tick's events, announcing the machine's layout
         * ahead of the first tick's.
         * @param tick The tick to fetch events for.
         * @return The wrapped source's events, with a game.bind_key
         * ahead of them for each binding the machine had changed, on the
         * first call only.
         */
        [[nodiscard]] std::vector<Event> eventsFor(
            antwika::time::Tick tick) override;

    private:
        ITickEventSource &inner;
        std::optional<KeyBindings> announced;
    };

} // namespace antwika::game
