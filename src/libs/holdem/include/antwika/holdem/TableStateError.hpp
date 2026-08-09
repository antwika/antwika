#pragma once

#include <stdexcept>

namespace antwika::holdem
{

    class TableStateError final : public std::runtime_error
    {
    public:
        using std::runtime_error::runtime_error;
    };

}
