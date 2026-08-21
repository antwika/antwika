#include "StbImage.hpp"

#define STB_IMAGE_STATIC
#define STB_IMAGE_IMPLEMENTATION

#include <stb_image.h>

namespace antwika::gfx::detail
{

    DecodedImage decodeImage(
        const unsigned char *bytes, int length, int desiredChannels)
    {
        DecodedImage decodedImage;
        int channels = 0;

        decodedImage.pixels = stbi_load_from_memory(
            bytes,
            length,
            &decodedImage.width,
            &decodedImage.height,
            &channels,
            desiredChannels);

        return decodedImage;
    }

    void freeDecodedImage(unsigned char *pixels)
    {
        stbi_image_free(pixels);
    }

    const char *decodeFailureReason()
    {
        return stbi_failure_reason();
    }

}
