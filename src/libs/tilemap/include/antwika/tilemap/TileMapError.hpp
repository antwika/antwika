#pragma once

#include <stdexcept>

namespace antwika::tilemap
{

    class TileMapError final : public std::runtime_error
    {
    public:
        using std::runtime_error::runtime_error;
    };

}
