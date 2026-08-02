#pragma once

#include <antwika/event/ITickEventSink.hpp>
#include <antwika/event/TickEvent.hpp>

#include "antwika/game/OptionsState.hpp"

namespace antwika::game
{

    using antwika::event::ITickEventSink;
    using antwika::event::TickEvent;

    /**
     * @brief Folds the machine's announced key bindings into the run.
     *
     * The other half of BindingSource, and the only reader of
     * game.bind_key. It is below the recorder like every other sink
     * here, so a live run and its replay fold exactly the same
     * announcements in exactly the same order.
     *
     * **Register it immediately after AppModeState**, ahead of anything
     * that reads a binding: the announcement arrives on the first tick,
     * before that tick's own input, and a sink reading a layout before
     * this one has folded it would read the layout of a run nobody was
     * playing.
     *
     * It is deliberately not gated on a mode. A binding is what a key
     * means on every screen, and the announcement arrives on the first
     * tick of a run that opens at the main menu.
     */
    class BindingSink final : public ITickEventSink
    {
    public:
        /**
         * @brief Construct the sink over the state it writes.
         * @param options The run's bindings; must outlive this sink.
         */
        explicit BindingSink(OptionsState &options) noexcept;

        BindingSink(const BindingSink &) = delete;
        BindingSink(BindingSink &&) = delete;

        BindingSink &operator=(const BindingSink &) = delete;
        BindingSink &operator=(BindingSink &&) = delete;

        /**
         * @brief Apply a tick event.
         * @param event A game.bind_key is folded; anything else is
         * ignored.
         * @throws OptionsFormatError If a game.bind_key carries a
         * payload of the wrong shape, or names an action or a key this
         * build does not know.
         */
        void handle(const TickEvent &event) override;

    private:
        OptionsState &options;
    };

} // namespace antwika::game
