#pragma once

#include <cstdint>

namespace antwika::atlas_editor
{

    /**
     * @brief One pixel of the image being edited, in image coordinates.
     *
     * Signed, because working a screen position back through the view
     * lands outside the image at least as often as inside it, and an
     * unsigned coordinate would wrap rather than say so.
     */
    struct Pixel
    {
        std::int32_t x = 0;
        std::int32_t y = 0;

        /**
         * @brief Compare two pixels.
         * @param other The pixel to compare against.
         * @return True when both coordinates match.
         */
        [[nodiscard]] bool operator==(const Pixel &other) const = default;
    };

} // namespace antwika::atlas_editor
