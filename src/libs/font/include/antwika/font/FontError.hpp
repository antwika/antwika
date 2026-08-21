#pragma once

#include <stdexcept>

namespace antwika::font
{

    class FontError final : public std::runtime_error
    {
    public:
        using std::runtime_error::runtime_error;
    };

}
