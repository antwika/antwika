#pragma once

namespace antwika::font
{

    struct FontMetrics final
    {
        int ascent = 0;
        int descent = 0;
        int lineGap = 0;
        int lineHeight = 0;

        [[nodiscard]] bool operator==(const FontMetrics &other) const
            = default;
    };

}
