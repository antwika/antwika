#include "antwika/tileset/PixelClass.hpp"

#include <string_view>

namespace antwika::tileset
{

    std::string_view toString(PixelClass pixel) noexcept
    {
        switch (pixel)
        {
            case PixelClass::Blank:
                return "blank";
            case PixelClass::Paper:
                return "paper";
            case PixelClass::Ink:
                return "ink";
        }

        return "unknown";
    }

}
