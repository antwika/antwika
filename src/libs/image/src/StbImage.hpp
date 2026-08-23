#pragma once

namespace antwika::image::detail
{

    struct DecodedImage final
    {
        unsigned char *pixels = nullptr;
        int width = 0;
        int height = 0;
    };

    [[nodiscard]] DecodedImage getDecodeImage(
        const unsigned char *bytes, int length, int desiredChannels);

    void freeDecodedImage(unsigned char *pixels);

    [[nodiscard]] const char *getDecodeFailureReason();

}
