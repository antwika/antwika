#pragma once

#include <antwika/event/ITickEventSink.hpp>

#include "antwika/game/GameState.hpp"

namespace antwika::game
{

    using antwika::event::ITickEventSink;
    using antwika::event::TickEvent;

    class GameStateReducer final : public ITickEventSink
    {
    public:
        explicit GameStateReducer(GameState &state);

        GameStateReducer(const GameStateReducer &) = delete;
        GameStateReducer(GameStateReducer &&) = delete;

        GameStateReducer &operator=(const GameStateReducer &) = delete;
        GameStateReducer &operator=(GameStateReducer &&) = delete;

        void handle(const TickEvent &event) override;

    private:
        GameState &state;
    };

}
