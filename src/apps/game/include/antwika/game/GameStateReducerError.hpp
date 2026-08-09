#pragma once

#include <stdexcept>

namespace antwika::game
{

    class GameStateReducerError final : public std::runtime_error
    {
    public:
        using std::runtime_error::runtime_error;
    };

}
