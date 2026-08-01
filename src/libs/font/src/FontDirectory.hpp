#pragma once

#include <cstdint>
#include <span>

namespace antwika::font::detail
{

    /**
     * @brief Refuse bytes that a rasteriser would read past the end of.
     *
     * stb_truetype trusts the offsets it is handed, which is the usual
     * bargain with a single-header decoder and is not one this library
     * can pass on to its callers: a font arrives from a file somebody
     * else wrote.
     * So the offset table and every table record are checked to lie
     * inside the blob before the rasteriser sees any of it, and the
     * flavours this library does not read outlines from are turned
     * away by name rather than left to fail somewhere less legible.
     *
     * What it deliberately does not do is validate a table's contents.
     * That is the rasteriser's job, it reports it by refusing to
     * initialise, and duplicating it here would be a second, weaker
     * parser to keep in step with the first.
     *
     * @param bytes The whole font file.
     * @throws FontError If the bytes are too short to hold an offset
     * table, name a flavour this library does not read, declare no
     * tables at all, carry a table directory that does not fit inside
     * them, or name a table that runs past their end.
     */
    void requireReadableDirectory(std::span<const std::uint8_t> bytes);

} // namespace antwika::font::detail
