#pragma once

#include <stdexcept>

namespace antwika::app
{

    class FramePacingError final : public std::runtime_error
    {
    public:
        using std::runtime_error::runtime_error;
    };

}
