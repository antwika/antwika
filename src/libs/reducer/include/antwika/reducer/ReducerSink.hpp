#pragma once

#include <antwika/event/ITimedEventSink.hpp>

#include "antwika/reducer/IReducer.hpp"

namespace antwika::reducer
{

    using antwika::event::ITimedEventSink;
    using antwika::event::TimedEvent;

    /**
     * @brief Makes any IReducer<State> pluggable into the engine as an
     * ITimedEventSink.
     *
     * The one generic adapter that replaces hand-writing a mutate-in-
     * place ITimedEventSink per application-state type: construct it
     * over a State and an IReducer<State>, and every dispatched event
     * folds through reduce() into the referenced state.
     */
    template <typename State>
    class ReducerSink final : public ITimedEventSink
    {
    public:
        /**
         * @brief Construct the sink from its collaborators.
         * @param state The state to update as events are handled. Must
         * outlive this object.
         * @param reducer Computes each event's effect on state. Must
         * outlive this object.
         */
        ReducerSink(State &state, const IReducer<State> &reducer)
            : state(state), reducer(reducer)
        {
        }

        ReducerSink(const ReducerSink &) = delete;
        ReducerSink(ReducerSink &&) = delete;

        ReducerSink &operator=(const ReducerSink &) = delete;
        ReducerSink &operator=(ReducerSink &&) = delete;

        /**
         * @brief Fold a dispatched event into the referenced state.
         * @param event The timed event to handle.
         */
        void handle(const TimedEvent &event) override
        {
            state = reducer.reduce(state, event);
        }

    private:
        State &state;
        const IReducer<State> &reducer;
    };

} // namespace antwika::reducer
