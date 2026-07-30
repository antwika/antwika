#pragma once

#include <cstdint>
#include <string_view>

#include "antwika/gfx/Size.hpp"

namespace antwika::gfx
{

    /**
     * @brief Measure the area a line of text will occupy.
     *
     * Pure arithmetic over the fixed-cell font's metrics rather than a
     * question for a backend, which is what lets a caller centre or
     * right-align text and still draw the same picture everywhere.
     * @param text The characters that would be drawn.
     * @param scale Pixels per glyph pixel, as passed to
     * IRenderer::drawText.
     * @return The width and height the text covers, including the gap
     * after its last glyph; zero for empty text or a zero scale.
     */
    [[nodiscard]] Size textSize(
        std::string_view text, std::uint32_t scale) noexcept;

} // namespace antwika::gfx
