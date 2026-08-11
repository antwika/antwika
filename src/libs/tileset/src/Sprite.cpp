#include "antwika/tileset/Sprite.hpp"

#include <algorithm>
#include <string_view>

#include "antwika/tileset/PixelClass.hpp"

namespace antwika::tileset
{

    bool isBlank(const SpriteFrame &frame) noexcept
    {
        return std::ranges::all_of(
            frame.pixels,
            [](const PixelClass pixel)
            { return pixel == PixelClass::Blank; });
    }

    std::string_view toString(Side side) noexcept
    {
        switch (side)
        {
            case Side::North:
                return "north";
            case Side::East:
                return "east";
            case Side::South:
                return "south";
            case Side::West:
                return "west";
        }

        return "unknown";
    }

}
