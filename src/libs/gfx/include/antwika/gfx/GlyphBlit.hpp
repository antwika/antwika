#pragma once

#include "antwika/gfx/Rect.hpp"

namespace antwika::gfx
{

    struct GlyphBlit final
    {
        Rect sourceRect;
        Rect destinationRect;

        [[nodiscard]] bool operator==(const GlyphBlit &other) const
            = default;
    };

}
