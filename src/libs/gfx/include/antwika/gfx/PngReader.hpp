#pragma once

#include <iosfwd>

#include "antwika/gfx/Bitmap.hpp"

namespace antwika::gfx
{

    /**
     * @brief Decodes a PNG byte stream into straight RGBA pixels.
     *
     * Takes a stream rather than a path, mirroring
     * antwika::replay::ReplayReader: antwika::gfx opens no files, so
     * every failure this can report is reachable from an in-memory
     * stream and provable without a fixture on disk.
     *
     * Every PNG colour type decodes to the same 8-bit RGBA layout --
     * greyscale, palette and truecolour all gain an opaque alpha
     * channel, and 16-bit channels are reduced to 8 -- so a caller never
     * has to ask what was in the file, and every backend uploads the
     * same bytes.
     */
    class PngReader final
    {
    public:
        /**
         * @brief Read and decode one PNG image.
         * @param in The stream to read to end-of-file.
         * @return The decoded pixels, always complete.
         * @throws GfxError If the stream is empty, holds something that
         * is not a PNG, is truncated, or describes an image too large to
         * hold.
         */
        [[nodiscard]] Bitmap read(std::istream &in) const;
    };

} // namespace antwika::gfx
