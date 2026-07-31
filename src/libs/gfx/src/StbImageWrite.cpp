#include "StbImageWrite.hpp"

#include <cstddef>
#include <string>

// The one place stb_image_write's implementation is compiled.
// STB_IMAGE_WRITE_STATIC keeps every stbiw_ symbol internal to this
// file, exactly as StbImage.cpp does for the decoder: raylib links its
// own copy, and two sets of them do not link.
// STBI_WRITE_NO_STDIO removes the path-taking entry points, so nothing
// in antwika::gfx can write a file even by accident.
// It is third-party C that does not build warning-clean either.
// So the CMakeLists silences warnings for this file alone, and the one
// wrapper below is kept trivial for that reason.
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
        // A failed encode never reaches the callback, so the one thing
        // it can leave behind is the empty string built above -- which
        // is what this promises to answer with, and one question rather
        // than two ways of asking the same one.
        static_cast<void>(stbi_write_png_to_func(
            &appendBytes,
            &encoded,
            width,
            height,
            channels,
            pixels,
            width * channels));

        return encoded;
    }

} // namespace antwika::gfx::detail
