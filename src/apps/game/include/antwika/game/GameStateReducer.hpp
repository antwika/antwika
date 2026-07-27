#pragma once

#include <antwika/event/ITimedEventSink.hpp>

#include "antwika/game/GameState.hpp"

namespace antwika::game
{

    using antwika::event::ITimedEventSink;
    using antwika::event::TimedEvent;

    // Folds tick-stamped events into GameState. Reacts to the engine's own
    // built-in tick event and to this application's custom
    // events::kScoreIncrement through the identical ITimedEventSink
    // mechanism -- proving both are usable the same way, with no
    // special-casing between "built-in" and "custom".
    class GameStateReducer final : public ITimedEventSink
    {
    public:
        explicit GameStateReducer(GameState &state);

        void handle(const TimedEvent &event) override;

    private:
        GameState &state;
    };

} // namespace antwika::game
