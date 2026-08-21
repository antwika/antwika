#pragma once

#include <antwika/geometry/Grid.hpp>
#include <antwika/gfx/Bitmap.hpp>
#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/RectF.hpp>
#include <antwika/gfx/Size.hpp>

namespace antwika::character
{

    struct PixelSelection final
    {
        geometry::GridCell fromCell{};

        geometry::GridCell toCell{};

        [[nodiscard]] bool operator==(
            const PixelSelection &other) const
            = default;
    };

}
