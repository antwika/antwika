#include "antwika/image/PngWriter.hpp"

#include <ios>
#include <ostream>
#include <string>

#include "antwika/gfx/Bitmap.hpp"
#include "antwika/gfx/GfxError.hpp"

#include "StbImageWrite.hpp"

namespace antwika::image
{

    void PngWriter::write(
        const gfx::Bitmap &bitmap, std::ostream &outputStream) const
    {
        if (!bitmap.isValid())
        {
            throw gfx::GfxError(
                "gfx: could not write a PNG: the bitmap does not hold "
                "the pixels it claims to");
        }

        const std::string encodedBytes = detail::getEncodePng(
            bitmap.pixels.data(),
            static_cast<int>(bitmap.size.width),
            static_cast<int>(bitmap.size.height),
            static_cast<int>(gfx::kBytesPerPixel));

        if (encodedBytes.empty()) // GCOVR_EXCL_LINE
        {
            throw gfx::GfxError(
                "gfx: could not encode a PNG"); // GCOVR_EXCL_LINE
        }

        outputStream.write(
            encodedBytes.data(),
            static_cast<std::streamsize>(encodedBytes.size()));

        outputStream.flush();

        if (!outputStream)
        {
            throw gfx::GfxError(
                "gfx: could not write a PNG: the stream would not take "
                "the encoded bytes");
        }
    }

}
