#pragma once

#include <stdexcept>

namespace antwika::sudoku
{

    class BoardFormatError final : public std::runtime_error
    {
    public:
        using std::runtime_error::runtime_error;
    };

}
