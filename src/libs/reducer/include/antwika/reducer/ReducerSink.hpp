#pragma once

#include <antwika/event/ITickEventSink.hpp>

#include "antwika/reducer/IReducer.hpp"

namespace antwika::reducer
{

    using antwika::event::ITickEventSink;
    using antwika::event::TickEvent;

    /**
     * @brief Makes any IReducer<State> pluggable into the engine as an
     * ITickEventSink.
     *
     * The one generic adapter that replaces hand-writing a mutate-in-
     * place ITickEventSink per application-state type: construct it
     * over a State and an IReducer<State>, and every dispatched event
     * folds through reduce() into the referenced state.
     */
    template <typename State>
    class ReducerSink final : public ITickEventSink
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
         * @param event The tick event to handle.
         */
        void handle(const TickEvent &event) override
        {
            state = reducer.reduce(state, event);
        }

    private:
        State &state;
        const IReducer<State> &reducer;
    };

} // namespace antwika::reducer
