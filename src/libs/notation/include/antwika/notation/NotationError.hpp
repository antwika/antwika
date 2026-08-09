#pragma once

#include <stdexcept>

namespace antwika::notation
{

    class NotationError final : public std::runtime_error
    {
    public:
        using std::runtime_error::runtime_error;
    };

}
