#pragma once

#include <stdexcept>

namespace antwika::life
{

    class BoardSinkError final : public std::runtime_error
    {
    public:
        using std::runtime_error::runtime_error;
    };

}
