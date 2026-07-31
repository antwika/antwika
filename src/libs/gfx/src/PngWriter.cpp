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

        // stb measures a dimension in a signed int, so a bitmap wider or
        // taller than one of those would wrap to some other size.
        // Being complete is what rules that out rather than a second
        // check: 2^31 pixels along one edge is 8 GiB of straight RGBA
        // before the other edge has a second row, and a bitmap that does
        // not hold every one of those bytes was refused above.
        const std::string encoded = detail::encodePng(
            bitmap.pixels.data(),
            static_cast<int>(bitmap.size.width),
            static_cast<int>(bitmap.size.height),
            static_cast<int>(kBytesPerPixel));

        // An allocation failure inside the encoder is the only way here:
        // it refuses nothing a complete bitmap can express.
        // That is case (a) of docs/confirming-unreachable-branches.md,
        // reachable only if a malloc actually fails.
        if (encoded.empty()) // GCOVR_EXCL_LINE
        {
            throw GfxError(
                "gfx: could not encode a PNG"); // GCOVR_EXCL_LINE
        }

        out.write(
            encoded.data(), static_cast<std::streamsize>(encoded.size()));

        // Checked rather than left to the caller, because a save that
        // half-wrote a file and said nothing is the one failure an
        // editor cannot recover from.
        if (!out)
        {
            throw GfxError(
                "gfx: could not write a PNG: the stream would not take "
                "the encoded bytes");
        }
    }

} // namespace antwika::gfx
