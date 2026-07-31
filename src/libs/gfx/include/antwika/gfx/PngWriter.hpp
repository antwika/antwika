#pragma once

#include <iosfwd>

#include "antwika/gfx/Bitmap.hpp"

namespace antwika::gfx
{

    /**
     * @brief Encodes straight RGBA pixels as a PNG byte stream.
     *
     * PngReader's counterpart, on exactly PngReader's terms: it takes a
     * stream rather than a path, because antwika::gfx opens no files, so
     * every failure this can report is reachable from an in-memory
     * stream and provable without a fixture on disk.
     *
     * What goes in is what came out: a bitmap read back through
     * PngReader is byte for byte the one written here, since both sides
     * speak the one 8-bit straight-RGBA layout Bitmap describes. There
     * is no colour type, no bit depth and no palette to choose, for the
     * reason the reader gives for collapsing every one of them on the
     * way in.
     */
    class PngWriter final
    {
    public:
        /**
         * @brief Encode one image and write it out.
         *
         * The argument order mirrors antwika::replay::ReplayWriter:
         * what is being written, then where it goes.
         *
         * @param bitmap The pixels to encode; must be complete.
         * @param out The stream to write the encoded file to.
         * @throws GfxError If the bitmap does not hold exactly the
         * pixels it claims to, or if the stream would not take the bytes
         * the encoder produced.
         */
        void write(const Bitmap &bitmap, std::ostream &out) const;
    };

} // namespace antwika::gfx
