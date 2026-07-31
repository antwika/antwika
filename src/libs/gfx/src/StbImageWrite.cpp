#include "StbImageWrite.hpp"

#include <cstddef>
#include <string>

// The one place stb_image_write's implementation is compiled.
// STB_IMAGE_WRITE_STATIC keeps every stbiw_ symbol internal here.
// StbImage.cpp does the same for the decoder, for the same reason.
// raylib links its own copy, and two sets of them do not link.
// STBI_WRITE_NO_STDIO removes the path-taking entry points.
// Nothing in antwika::gfx can then write a file even by accident.
// It is third-party C that does not build warning-clean either.
// So the CMakeLists silences warnings for this file alone.
// The one wrapper below is kept trivial for that reason.
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
    } // namespace

    std::string encodePng(
        const unsigned char *pixels,
        const int width,
        const int height,
        const int channels)
    {
        std::string encoded;

        // The result is deliberately not looked at.
        // A failed encode never reaches the callback.
        // So the one thing it leaves behind is the empty string above.
        // That is what this promises to answer with.
        // One question, rather than two ways of asking the same one.
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

} // namespace antwika::gfx::detail
