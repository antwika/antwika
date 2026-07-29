#pragma once

#include <antwika/event/TickEvent.hpp>

namespace antwika::reducer
{

    using antwika::event::TickEvent;

    /**
     * @brief A pure function from (previous state, event) to next state.
     *
     * No mutation, no side effects: reduce() returns a new State rather
     * than changing one in place, the same "don't directly affect state"
     * discipline antwika::ecs applies to systems, applied here to
     * plain-struct application state. State must be copyable.
     */
    template <typename State>
    class IReducer
    {
    public:
        virtual ~IReducer() = default;

        /**
         * @brief Fold one event into a state value.
         * @param previous The state before this event.
         * @param event The event to fold in.
         * @return The state after this event.
         */
        [[nodiscard]] virtual State reduce(
            const State &previous, const TickEvent &event) const = 0;
    };

} // namespace antwika::reducer
