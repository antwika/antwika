#pragma once

#include <iosfwd>

#include "antwika/gfx/Bitmap.hpp"

namespace antwika::gfx
{

    class PngWriter final
    {
    public:
        void write(const Bitmap &bitmap, std::ostream &outputStream) const;
    };

}
