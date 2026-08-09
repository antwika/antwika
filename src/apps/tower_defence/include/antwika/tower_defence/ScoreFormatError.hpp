#pragma once

#include <stdexcept>

namespace antwika::tower_defence
{

    class ScoreFormatError final : public std::runtime_error
    {
    public:
        using std::runtime_error::runtime_error;
    };

}
