#pragma once

#include <stdexcept>

namespace antwika::cli
{

    class CommandLineError final : public std::runtime_error
    {
    public:
        using std::runtime_error::runtime_error;
    };

}
