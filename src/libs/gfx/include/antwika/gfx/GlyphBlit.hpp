#pragma once

#include "antwika/gfx/Rect.hpp"

namespace antwika::gfx
{

    struct GlyphBlit final
    {
        Rect source;
        Rect destination;

        [[nodiscard]] bool operator==(const GlyphBlit &other) const
            = default;
    };

}
