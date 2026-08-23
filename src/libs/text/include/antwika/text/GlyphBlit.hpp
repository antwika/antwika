#pragma once

#include "antwika/gfx/Rect.hpp"

namespace antwika::text
{

    struct GlyphBlit final
    {
        gfx::Rect sourceRect;
        gfx::Rect destinationRect;

        [[nodiscard]] bool operator==(const GlyphBlit &other) const
            = default;
    };

}
