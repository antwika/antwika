#pragma once

namespace antwika::gfx
{

    struct GfxCapabilities final
    {
        bool readsPixels = false;

        bool resizesWindows = false;

        [[nodiscard]] bool operator==(
            const GfxCapabilities &other) const = default;
    };

}
