#pragma once

#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/ITexture.hpp>
#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/Rect.hpp>

namespace antwika::ui
{

    struct PopClip final
    {
        [[nodiscard]] bool operator==(const PopClip &other) const =
            default;
    };

}
