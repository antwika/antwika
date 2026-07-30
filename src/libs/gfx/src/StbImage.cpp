#include "StbImage.hpp"

// The one place stb_image's implementation is compiled.
// STB_IMAGE_STATIC keeps every stbi_ symbol internal to this file.
// raylib links its own copy, and two sets of them do not link.
// It is third-party C that does not build warning-clean either.
// So the CMakeLists silences warnings for this file alone.
// The three wrappers below are kept trivial for that reason.
#define STB_IMAGE_STATIC
#define STB_IMAGE_IMPLEMENTATION

#include <stb_image.h>

namespace antwika::gfx::detail
{

    DecodedImage decodeImage(
        const unsigned char *bytes, int length, int desiredChannels)
    {
        DecodedImage decoded;
        int channels = 0;

        decoded.pixels = stbi_load_from_memory(
            bytes,
            length,
            &decoded.width,
            &decoded.height,
            &channels,
            desiredChannels);

        return decoded;
    }

    void freeDecodedImage(unsigned char *pixels)
    {
        stbi_image_free(pixels);
    }

    const char *decodeFailureReason()
    {
        return stbi_failure_reason();
    }

} // namespace antwika::gfx::detail
