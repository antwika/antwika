#pragma once

#include <antwika/event/ITickEventSink.hpp>

#include "antwika/game/GameState.hpp"

namespace antwika::game
{

    using antwika::event::ITickEventSink;
    using antwika::event::TickEvent;

    /**
     * @brief Folds tick-stamped events into GameState.
     *
     * Reacts to both the engine's own built-in tick event and this
     * application's custom events::kScoreIncrement, through the identical
     * ITickEventSink mechanism — there is no special-casing between
     * "built-in" and "custom" events.
     */
    class GameStateReducer final : public ITickEventSink
    {
    public:
        /**
         * @brief Construct the reducer over the state it will mutate.
         * @param state The state to update as events are handled.
         * Must outlive this object.
         */
        explicit GameStateReducer(GameState &state);

        GameStateReducer(const GameStateReducer &) = delete;
        GameStateReducer(GameStateReducer &&) = delete;

        GameStateReducer &operator=(const GameStateReducer &) = delete;
        GameStateReducer &operator=(GameStateReducer &&) = delete;

        /**
         * @brief Apply a tick event's effect to the referenced GameState.
         * @param event The event to fold in. kTick increments ticksProcessed;
         * kScoreIncrement adds its payload (a base-10 integer) to score.
         */
        void handle(const TickEvent &event) override;

    private:
        GameState &state;
    };

} // namespace antwika::game
