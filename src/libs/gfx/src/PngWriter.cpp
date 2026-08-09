#include "antwika/gfx/PngWriter.hpp"

#include <ios>
#include <ostream>
#include <string>

#include "antwika/gfx/Bitmap.hpp"
#include "antwika/gfx/GfxError.hpp"

#include "StbImageWrite.hpp"

namespace antwika::gfx
{

    void PngWriter::write(const Bitmap &bitmap, std::ostream &out) const
    {
        if (!bitmap.isComplete())
        {
            throw GfxError(
                "gfx: could not write a PNG: the bitmap does not hold "
                "the pixels it claims to");
        }

        const std::string encoded = detail::encodePng(
            bitmap.pixels.data(),
            static_cast<int>(bitmap.size.width),
            static_cast<int>(bitmap.size.height),
            static_cast<int>(kBytesPerPixel));

        if (encoded.empty()) // GCOVR_EXCL_LINE
        {
            throw GfxError(
                "gfx: could not encode a PNG"); // GCOVR_EXCL_LINE
        }

        out.write(
            encoded.data(), static_cast<std::streamsize>(encoded.size()));

        out.flush();

        if (!out)
        {
            throw GfxError(
                "gfx: could not write a PNG: the stream would not take "
                "the encoded bytes");
        }
    }

}
