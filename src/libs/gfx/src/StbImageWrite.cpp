#include "StbImageWrite.hpp"

#include <cstddef>
#include <string>

#define STB_IMAGE_WRITE_STATIC
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STBI_WRITE_NO_STDIO

#include <stb_image_write.h>

namespace antwika::gfx::detail
{

    namespace
    {
        void appendBytes(void *context, void *bytes, int length)
        {
            auto *encoded = static_cast<std::string *>(context);

            encoded->append(
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
        std::string encoded;

        static_cast<void>(stbi_write_png_to_func(
            &appendBytes,
            &encoded,
            width,
            height,
            channels,
            pixels,
            width * channels));

        return encoded;
    } // GCOVR_EXCL_LINE

}
