#pragma once

#include <string>

namespace antwika::gfx::detail
{

    /**
     * @brief Encode pixels held in memory as a PNG file's bytes.
     *
     * StbImage.hpp's counterpart, wrapping stb_image_write so that
     * exactly one translation unit compiles it -- for the same reason
     * the decoder has one: raylib links its own copy, and two sets of
     * stbiw_ symbols in one program is a multiple-definition link error.
     *
     * @param pixels The image, row by row with no padding between rows.
     * @param width How many pixels wide it is.
     * @param height How many rows it has.
     * @param channels How many bytes one pixel occupies.
     * @return The encoded file, or nothing at all when the encoder could
     * not allocate what it needed -- it calls nothing back on the way
     * out of a failure, so an empty result is what failure looks like.
     */
    [[nodiscard]] std::string encodePng(
        const unsigned char *pixels, int width, int height, int channels);

} // namespace antwika::gfx::detail
