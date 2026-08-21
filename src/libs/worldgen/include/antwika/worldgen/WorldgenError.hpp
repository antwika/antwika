#pragma once

#include <stdexcept>

namespace antwika::worldgen
{

    class WorldgenError final : public std::runtime_error
    {
        using std::runtime_error::runtime_error;
    };

}
