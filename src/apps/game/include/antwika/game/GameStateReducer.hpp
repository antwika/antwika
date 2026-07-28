#pragma once

#include <antwika/event/ITimedEventSink.hpp>

#include "antwika/game/GameState.hpp"

namespace antwika::game
{

    using antwika::event::ITimedEventSink;
    using antwika::event::TimedEvent;

    /**
     * @brief Folds tick-stamped events into GameState.
     *
     * Reacts to both the engine's own built-in tick event and this
     * application's custom events::kScoreIncrement, through the identical
     * ITimedEventSink mechanism — there is no special-casing between
     * "built-in" and "custom" events.
     */
    class GameStateReducer final : public ITimedEventSink
    {
    public:
        /**
         * @brief Construct the reducer over the state it will mutate.
         * @param state The state to update as events are handled. Must outlive this object.
         */
        explicit GameStateReducer(GameState &state);

        /**
         * @brief Apply a timed event's effect to the referenced GameState.
         * @param event The event to fold in. kTick increments ticksProcessed;
         * kScoreIncrement adds its payload (a base-10 integer) to score.
         */
        void handle(const TimedEvent &event) override;

    private:
        GameState &state;
    };

} // namespace antwika::game
