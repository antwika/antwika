#pragma once

#include <stdexcept>

namespace antwika::replay
{

    class ReplayFormatError : public std::runtime_error
    {
    public:
        using std::runtime_error::runtime_error;
    };

}
