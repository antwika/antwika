#include "antwika/game/GameStateReducer.hpp"

#include <string>

#include <antwika/engine/Events.hpp>

#include "antwika/game/Events.hpp"

namespace antwika::game
{

    GameStateReducer::GameStateReducer(GameState &state) : state(state)
    {
    }

    void GameStateReducer::handle(const TimedEvent &event)
    {
        if (event.event.name == antwika::engine::events::kTick)
        {
            ++state.ticksProcessed;
        }
        else if (event.event.name == events::kScoreIncrement)
        {
            state.score += std::stoull(event.event.payload);
        }
    }

} // namespace antwika::game
