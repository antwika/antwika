#pragma once

#include <string>
#include <string_view>

#include <antwika/sound/Waveform.hpp>

namespace antwika::app
{

    /**
     * @brief Read a WAV file off disk as a waveform.
     *
     * Opening the file is the application's job, not the sound library's:
     * antwika::sound decodes bytes and never goes looking for them, which
     * is why saying a file is missing is an application's job too. Every
     * application that says it says it the same way, so it is said here.
     *
     * The line-for-line counterpart of readPngFile, on purpose: the two
     * libraries take the same position about files, so the two callers
     * should not have to be read differently to see it.
     *
     * @param path The file to read.
     * @param name The program's name, used to prefix a failure.
     * @return The decoded audio.
     * @throws antwika::sound::SoundError If the file cannot be opened, or
     * its bytes are not a WAV this can decode.
     */
    [[nodiscard]] antwika::sound::Waveform readWavFile(
        const std::string &path, std::string_view name);

} // namespace antwika::app
