#pragma once

#include <stdexcept>

namespace antwika::config
{

    class ConfigFormatError final : public std::runtime_error
    {
    public:
        using std::runtime_error::runtime_error;
    };

}
