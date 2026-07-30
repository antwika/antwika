#pragma once

namespace antwika::gfx::detail
{

    /**
     * @brief What stb_image made of one attempt to decode an image.
     */
    struct DecodedImage
    {
        unsigned char *pixels = nullptr;
        int width = 0;
        int height = 0;
    };

    /**
     * @brief Decode an image held in memory.
     *
     * Wraps stb_image so that exactly one translation unit compiles it.
     * That matters beyond tidiness: raylib links its own copy of
     * stb_image, and two sets of stbi_ symbols in one program is a
     * multiple-definition link error.
     * Keeping the implementation static and behind this wrapper leaves
     * ours invisible to the linker.
     * @param bytes The encoded image.
     * @param length How many bytes long it is.
     * @param desiredChannels Channels per pixel to convert to.
     * @return The pixels, or a null pixels pointer on failure.
     */
    [[nodiscard]] DecodedImage decodeImage(
        const unsigned char *bytes, int length, int desiredChannels);

    /**
     * @brief Release pixels returned by decodeImage().
     * @param pixels The pixels, which may be null.
     */
    void freeDecodedImage(unsigned char *pixels);

    /**
     * @brief Explain the most recent decode failure.
     * @return A description owned by the decoder, never null.
     */
    [[nodiscard]] const char *decodeFailureReason();

} // namespace antwika::gfx::detail
