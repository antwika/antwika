#pragma once

#include <iosfwd>

#include "antwika/sound/Waveform.hpp"

namespace antwika::sound
{

    /**
     * @brief Decodes a WAV byte stream into normalised float samples.
     *
     * Takes a stream rather than a path, exactly as gfx::PngReader does
     * and for the same reason: antwika::sound opens no files, so every
     * failure this can report is reachable from an in-memory stream and
     * provable without a fixture on disk.
     *
     * **Hand-rolled rather than a dependency**, which is the opposite of
     * the call made for PNG. PNG is DEFLATE plus per-row filters plus
     * five colour types; WAV is a walk over chunks. The deciding
     * argument is the coverage gate: a third-party decoder's error paths
     * are frequently unreachable from a caller, and every throw below is
     * reachable from a handful of crafted bytes.
     *
     * Every storage format decodes to the same normalised float layout,
     * so a caller never has to ask what was in the file.
     */
    class WavReader final
    {
    public:
        /**
         * @brief Read and decode one WAV file.
         * @param in The stream to read to end-of-file.
         * @return The decoded audio, always complete.
         * @throws SoundError If the stream is empty, is not RIFF/WAVE,
         * is truncated, names a compression this does not decode, or
         * describes audio it could not hold.
         */
        [[nodiscard]] Waveform read(std::istream &in) const;
    };

} // namespace antwika::sound
