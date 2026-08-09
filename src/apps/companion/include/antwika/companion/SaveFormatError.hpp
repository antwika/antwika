#pragma once

#include <stdexcept>

namespace antwika::companion
{

    class SaveFormatError final : public std::runtime_error
    {
    public:
        using std::runtime_error::runtime_error;
    };

}
