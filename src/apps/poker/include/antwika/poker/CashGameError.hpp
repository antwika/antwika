#pragma once

#include <stdexcept>

namespace antwika::poker
{

    class CashGameError final : public std::runtime_error
    {
    public:
        using std::runtime_error::runtime_error;
    };

}
