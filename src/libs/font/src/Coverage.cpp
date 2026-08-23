#include "antwika/font/Coverage.hpp"

#include <cstddef>
#include <cstdint>
#include <string>

#include "antwika/font/FontError.hpp"

namespace antwika::font
{

    bool Coverage::isValid() const noexcept
    {
        return samples.size()
            == static_cast<std::size_t>(width) * height;
    }

    std::uint8_t Coverage::getEntryAt(std::uint32_t x, std::uint32_t y) const
    {
        if (x >= width || y >= height || !isValid())
        {
            throw FontError(
                "font: no sample at " + std::to_string(x) + ","
                + std::to_string(y) + " of a "
                + std::to_string(width) + "x"
                + std::to_string(height) + " coverage mask");
        }

        return samples[static_cast<std::size_t>(y) * width + x];
    }

}
