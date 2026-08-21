#pragma once

#include <vector>
#include <antwika/geometry/Grid.hpp>
#include <antwika/gfx/Bitmap.hpp>
#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/RectF.hpp>
#include <antwika/gfx/Size.hpp>

namespace antwika::character
{

    struct PixelBuffer final
    {
        gfx::Size size{};

        std::vector<gfx::Color> pixelColors{};

        [[nodiscard]] bool operator==(const PixelBuffer &other) const
            = default;
    };

}
