#pragma once

#include <iosfwd>

#include "antwika/ttf/Font.hpp"

namespace antwika::ttf
{

    /**
     * @brief Reads a TrueType byte stream into a Font.
     *
     * Takes a stream rather than a path, exactly as gfx::PngReader and
     * sound::WavReader do and for the same two reasons: antwika::ttf
     * opens no files, and every failure it can report is therefore
     * reachable from an in-memory stream and provable without a font
     * checked into the repository.
     */
    class TtfReader final
    {
    public:
        /**
         * @brief Read and parse one font.
         * @param in The stream to read to end-of-file.
         * @return The parsed font.
         * @throws TtfError If the stream does not hold a font this
         * library can draw with.
         */
        [[nodiscard]] Font read(std::istream &in) const;
    };

} // namespace antwika::ttf
