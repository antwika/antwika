#include "antwika/gfx/Bitmap.hpp"

#include <cstddef>

namespace antwika::gfx
{

    bool Bitmap::isComplete() const noexcept
    {
        if (size.width == 0 || size.height == 0)
        {
            return false;
        }

        const std::size_t expected = static_cast<std::size_t>(size.width)
            * static_cast<std::size_t>(size.height) * kBytesPerPixel;

        return pixels.size() == expected;
    }

}
