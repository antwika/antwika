#include "StbImageWrite.hpp"

#include <cstddef>
#include <string>

#define STB_IMAGE_WRITE_STATIC
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STBI_WRITE_NO_STDIO

#include <stb_image_write.h>

namespace antwika::image::detail
{

    namespace
    {
        void appendBytes(void *context, void *bytes, int length)
        {
            auto *encodedBytes = static_cast<std::string *>(context);

            encodedBytes->append(
                static_cast<const char *>(bytes),
                static_cast<std::size_t>(length));
        }
    }

    std::string encodePng(
        const unsigned char *pixels,
        const int width,
        const int height,
        const int channels)
    {
        std::string encodedBytes;

        static_cast<void>(stbi_write_png_to_func(
            &appendBytes,
            &encodedBytes,
            width,
            height,
            channels,
            pixels,
            width * channels));

        return encodedBytes;
    } // GCOVR_EXCL_LINE

}
