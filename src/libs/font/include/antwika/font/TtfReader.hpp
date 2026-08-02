#pragma once

#include <iosfwd>

#include "antwika/font/Font.hpp"

namespace antwika::font
{

    /**
     * @brief Reads a TrueType byte stream into a Font.
     *
     * Takes a stream rather than a path, exactly as gfx::PngReader and
     * sound::WavReader do and for the same two reasons: antwika::font
     * opens no files, and every failure it can report is therefore
     * reachable from an in-memory stream and provable without a font
     * checked into the repository.
     *
     * **Feed this trusted, bundled assets only.** A font an application
     * ships beside itself with antwika_bundle_app(), or one compiled in
     * with antwika_embed_binary(), is what this is for. A font
     * downloaded, uploaded, or otherwise chosen by somebody else is
     * not, and no refusal here makes it so.
     *
     * The reason is where the parsing happens. What this checks is the
     * offset table and the table directory, so a record claiming a
     * table that runs past the end of the blob is a FontError before
     * anything else reads a byte. Everything inside a table is read
     * later and by stb_truetype, which states outright that it does no
     * range checking of its own: a cmap subtable offset is followed
     * when Font::has() or a glyph lookup asks for a codepoint, and
     * loca/glyf extents are followed when a glyph is rasterised. Both
     * are long after read() returned successfully, so a font crafted to
     * pass the directory check reads out of bounds mid-draw rather than
     * refusing to open.
     *
     * Validating those offsets here was considered and refused: it
     * would be a second, weaker parser of the same tables, kept in step
     * with stb's by hand, and a gap between the two would read as
     * safety while being none. The honest boundary is this sentence
     * instead -- an application decides where its fonts come from, and
     * that decision is the check.
     */
    class TtfReader final
    {
    public:
        /**
         * @brief Read and parse one font.
         * @param in The stream to read to end-of-file.
         * @return The parsed font.
         * @throws FontError If the stream does not hold a font this
         * library can draw with.
         */
        [[nodiscard]] Font read(std::istream &in) const;
    };

} // namespace antwika::font
