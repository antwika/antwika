#pragma once

#include <stdexcept>

namespace antwika::game
{

    /**
     * @brief Thrown by GameStateReducer when a game.score_increment
     * payload is not valid JSON, or not an object with an unsigned
     * integer "amount" field.
     */
    class GameStateReducerError final : public std::runtime_error
    {
    public:
        using std::runtime_error::runtime_error;
    };

} // namespace antwika::game
