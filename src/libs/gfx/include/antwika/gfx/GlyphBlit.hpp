#pragma once

#include "antwika/gfx/Rect.hpp"

namespace antwika::gfx
{

    /**
     * @brief One glyph taken out of an atlas texture and put somewhere
     * on the canvas.
     *
     * A plain value rather than a drawing call, for the reason
     * ui::DrawList is a value: a whole line of text can then be
     * asserted with EXPECT_EQ and no mock, and whatever paints it is a
     * loop with nothing to decide.
     * Both rectangles are the same size unless a caller scales one, so
     * an atlas built at the pixel height it is drawn at samples one
     * texel per pixel.
     */
    struct GlyphBlit
    {
        Rect source;
        Rect destination;

        /**
         * @brief Compare two blits.
         * @param other The blit to compare against.
         * @return True when both rectangles match.
         */
        [[nodiscard]] bool operator==(const GlyphBlit &other) const
            = default;
    };

} // namespace antwika::gfx
