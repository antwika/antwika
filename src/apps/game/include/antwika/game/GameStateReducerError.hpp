#pragma once

#include <stdexcept>

namespace antwika::game
{

    /**
     * @brief Thrown by GameStateReducer when a game.score_increment
     * payload is not a plain, in-range base-10 unsigned integer.
     */
    class GameStateReducerError final : public std::runtime_error
    {
    public:
        using std::runtime_error::runtime_error;
    };

} // namespace antwika::game
