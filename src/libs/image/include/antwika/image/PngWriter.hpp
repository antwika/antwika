#pragma once

#include <iosfwd>

#include "antwika/gfx/Bitmap.hpp"

namespace antwika::image
{

    class PngWriter final
    {
    public:
        void write(const gfx::Bitmap &bitmap, std::ostream &outputStream) const;
    };

}
