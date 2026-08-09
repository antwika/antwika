#pragma once

#include <stdexcept>

namespace antwika::input
{

    class InputError final : public std::runtime_error
    {
    public:
        using std::runtime_error::runtime_error;
    };

}
