#pragma once

namespace antwika::gfx::detail
{

    struct DecodedImage final
    {
        unsigned char *pixels = nullptr;
        int width = 0;
        int height = 0;
    };

    [[nodiscard]] DecodedImage decodeImage(
        const unsigned char *bytes, int length, int desiredChannels);

    void freeDecodedImage(unsigned char *pixels);

    [[nodiscard]] const char *decodeFailureReason();

}
