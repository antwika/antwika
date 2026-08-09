#pragma once

#include <iosfwd>

#include "antwika/gfx/Bitmap.hpp"

namespace antwika::gfx
{

    class PngReader final
    {
    public:
        [[nodiscard]] Bitmap read(std::istream &in) const;
    };

}
