#pragma once

#include <stdexcept>

namespace antwika::network
{

    class NetworkError final : public std::runtime_error
    {
    public:
        using std::runtime_error::runtime_error;
    };

}
