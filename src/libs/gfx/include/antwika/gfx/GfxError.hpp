#pragma once

#include <stdexcept>

namespace antwika::gfx
{

    class GfxError final : public std::runtime_error
    {
    public:
        using std::runtime_error::runtime_error;
    };

}
