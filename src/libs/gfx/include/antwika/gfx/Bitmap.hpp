#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "antwika/gfx/Size.hpp"

namespace antwika::gfx
{

    /**
     * @brief How many bytes one pixel of a Bitmap occupies.
     */
    inline constexpr std::size_t kBytesPerPixel = 4;

    /**
     * @brief Decoded, uncompressed pixels ready to become a texture.
     *
     * Straight (non-premultiplied) 8-bit RGBA in memory order, matching
     * Color's channels, laid out row by row from the top-left corner
     * with no padding between rows.
     * Deliberately a plain value: a bitmap is decoded once, handed to
     * IRenderer::createTexture, and is not aliased by the texture
     * afterwards, so nothing here owns a resource and nothing here has a
     * lifetime rule.
     */
    struct Bitmap
    {
        Size size;
        std::vector<std::uint8_t> pixels;

        /**
         * @brief Check that this bitmap holds exactly the pixels it
         * claims to.
         *
         * Every backend has to make this check before uploading, so it
         * lives here rather than three times over.
         * @return True when both dimensions are non-zero and pixels
         * holds width * height * kBytesPerPixel bytes.
         */
        [[nodiscard]] bool isComplete() const noexcept;

        /**
         * @brief Compare two bitmaps.
         * @param other The bitmap to compare against.
         * @return True when the sizes match and every pixel byte does.
         */
        [[nodiscard]] bool operator==(const Bitmap &other) const = default;
    };

} // namespace antwika::gfx
