#pragma once

#include <stdexcept>

namespace antwika::poker
{

    class BankrollError final : public std::runtime_error
    {
    public:
        using std::runtime_error::runtime_error;
    };

}
