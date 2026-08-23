#pragma once

#include <iosfwd>

#include "antwika/gfx/Bitmap.hpp"

namespace antwika::image
{

    class PngReader final
    {
    public:
        [[nodiscard]] gfx::Bitmap read(std::istream &inputStream) const;
    };

}
