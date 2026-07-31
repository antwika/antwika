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

        // stb measures a dimension in a signed int.
        // A bitmap past that would wrap to some other size.
        // Being complete is what rules that out, rather than a check.
        // One edge of 2^31 pixels is 8 GiB before the other has a row.
        // A bitmap not holding every one of those bytes was refused.
        const std::string encoded = detail::encodePng(
            bitmap.pixels.data(),
            static_cast<int>(bitmap.size.width),
            static_cast<int>(bitmap.size.height),
            static_cast<int>(kBytesPerPixel));

        // The encoder refuses nothing a complete bitmap can express.
        // So an allocation failure inside it is the only way here.
        // That is case (a) of docs/confirming-unreachable-branches.md.
        // It is reachable only if a malloc actually fails.
        if (encoded.empty()) // GCOVR_EXCL_LINE
        {
            throw GfxError(
                "gfx: could not encode a PNG"); // GCOVR_EXCL_LINE
        }

        out.write(
            encoded.data(), static_cast<std::streamsize>(encoded.size()));

        // Flushed here rather than left to the stream's destructor.
        // A destructor reports nothing at all.
        // A file small enough for one buffer would otherwise be lost.
        // A save that loses a sheet quietly is unrecoverable.
        out.flush();

        if (!out)
        {
            throw GfxError(
                "gfx: could not write a PNG: the stream would not take "
                "the encoded bytes");
        }
    }

} // namespace antwika::gfx
